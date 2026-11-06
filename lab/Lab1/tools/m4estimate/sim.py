"""Run one function of an ARM ELF in Unicorn and count Cortex-M4 cost features."""
import sys, json
from elftools.elf.elffile import ELFFile
from unicorn import *
from unicorn.arm_const import *
from capstone import *
from capstone.arm import *

STACK_TOP = 0x1FFE0000 + 640*1024 - 16
RET = 0x00FFFF00  # magic return address in flash space

def load(path):
    from objload import load_obj
    segs, syms = load_obj(path)
    mu = Uc(UC_ARCH_ARM, UC_MODE_THUMB | UC_MODE_MCLASS)
    mu.mem_map(0, 16 << 20)
    mu.mem_map(0x1FFE0000, 1 << 20)
    for a, b in segs:
        mu.mem_write(a, b)
    return mu, syms

def call(mu, addr, counters=None):
    mu.reg_write(UC_ARM_REG_SP, STACK_TOP)
    mu.reg_write(UC_ARM_REG_LR, RET | 1)
    if counters is None:
        mu.emu_start(addr | 1, RET, count=50_000_000)
        return
    md = Cs(CS_ARCH_ARM, CS_MODE_THUMB | CS_MODE_MCLASS); md.detail = True
    cache = {}
    c = counters
    state = {'prev': None}
    def classify(addr, size):
        ins = next(md.disasm(bytes(mu.mem_read(addr, size)), addr))
        m = ins.mnemonic.split('.')[0]
        g = ins.groups
        is_br = ARM_GRP_JUMP in g or m in ('b','bl','blx','bx','cbz','cbnz') or \
                (m in ('pop','ldm','ldr') and any(op.type == ARM_OP_REG and op.reg == ARM_REG_PC for op in ins.operands))
        kind = 'br' if is_br else ('mul' if m in ('mul','muls','mla','mls','smull','umull') else
                ('div' if m in ('sdiv','udiv') else 'alu'))
        return (kind, size, m)
    def hook_code(uc, addr, size, _):
        p = state['prev']
        if p is not None:
            paddr, psize, pkind = p
            if pkind == 'br' and addr != paddr + psize:
                c['br_taken'] += 1
        info = cache.get(addr)
        if info is None:
            info = cache[addr] = classify(addr, size)
        kind = info[0]
        c['ins'] += 1
        c['bytes'] += size
        if size == 4: c['wide'] += 1
        if addr & 2 and size == 4: c['unal'] += 1
        c[kind] += 1
        state['prev'] = (addr, size, kind)
    def hook_read(uc, access, addr, size, value, _):
        if addr < 0x1FFE0000: c['ld_flash'] += 1
        else: c['ld_ram'] += 1
    def hook_write(uc, access, addr, size, value, _):
        c['st'] += 1
    h = [mu.hook_add(UC_HOOK_CODE, hook_code),
         mu.hook_add(UC_HOOK_MEM_READ, hook_read),
         mu.hook_add(UC_HOOK_MEM_WRITE, hook_write)]
    mu.emu_start(addr | 1, RET, count=50_000_000)
    for x in h: mu.hook_del(x)

def measure(path, fn):
    mu, syms = load(path)
    call(mu, syms['setup'])
    c = dict(bytes=0, wide=0, unal=0, ins=0, alu=0, br=0, br_taken=0, mul=0, div=0, ld_flash=0, ld_ram=0, st=0)
    call(mu, syms[fn], c)
    return c

if __name__ == '__main__':
    out = {}
    import glob, os
    for p in sorted(glob.glob('obj/theirs_*.o')):
        out[os.path.basename(p)[:-2]] = measure(p, 'bench')
        print(os.path.basename(p), out[os.path.basename(p)[:-2]], flush=True)
    mu, syms = load('obj/ours.o')
    for name in sorted(s for s in syms if s.startswith('bench_')):
        out['ours_' + name[6:]] = measure('obj/ours.o', name)
        print(name, out['ours_' + name[6:]], flush=True)
    json.dump(out, open('features.json', 'w'), indent=1)
