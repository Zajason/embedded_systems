"""Load a relocatable ARM ELF (.o) into Unicorn: .text/.rodata in flash, .data/.bss in RAM."""
import struct
from elftools.elf.elffile import ELFFile

FLASH, RAM = 0x1000, 0x1FFE0000

def _movw_get(h1, h2):
    return ((h1 & 0xF) << 12) | (((h1 >> 10) & 1) << 11) | (((h2 >> 12) & 7) << 8) | (h2 & 0xFF)

def _movw_set(h1, h2, v):
    h1 = (h1 & 0xFBF0) | ((v >> 12) & 0xF) | (((v >> 11) & 1) << 10)
    h2 = (h2 & 0x8F00) | (((v >> 8) & 7) << 12) | (v & 0xFF)
    return h1, h2

def _bl_get(h1, h2):
    s = (h1 >> 10) & 1; imm10 = h1 & 0x3FF; j1 = (h2 >> 13) & 1; j2 = (h2 >> 11) & 1; imm11 = h2 & 0x7FF
    i1 = 1 - (j1 ^ s); i2 = 1 - (j2 ^ s)
    v = (s << 24) | (i1 << 23) | (i2 << 22) | (imm10 << 12) | (imm11 << 1)
    return v - (1 << 25) if s else v

def _bl_set(h1, h2, off):
    off &= (1 << 25) - 1
    s = (off >> 24) & 1; i1 = (off >> 23) & 1; i2 = (off >> 22) & 1
    j1 = 1 - (i1 ^ s); j2 = 1 - (i2 ^ s)
    h1 = (h1 & 0xF800) | (s << 10) | ((off >> 12) & 0x3FF)
    h2 = (h2 & 0xD000) | (j1 << 13) | (j2 << 11) | ((off >> 1) & 0x7FF)
    return h1, h2

def load_obj(path):
    e = ELFFile(open(path, 'rb'))
    base, img = {}, {}
    fa, ra = FLASH, RAM
    for i, s in enumerate(e.iter_sections()):
        if not (s['sh_flags'] & 2) or s.name.startswith('.ARM'):
            continue
        al = max(s['sh_addralign'], 4)
        if s['sh_flags'] & 1 or s['sh_type'] == 'SHT_NOBITS':   # writable -> RAM
            ra = (ra + al - 1) // al * al; base[i] = ra; ra += s['sh_size']
        else:
            fa = (fa + al - 1) // al * al; base[i] = fa; fa += s['sh_size']
        img[i] = bytearray(s.data()) if s['sh_type'] != 'SHT_NOBITS' else bytearray(s['sh_size'])
    symtab = e.get_section_by_name('.symtab')
    syms = list(symtab.iter_symbols())
    def symaddr(sym):
        sh = sym['st_shndx']
        if sh == 'SHN_UNDEF': raise KeyError('undefined symbol ' + sym.name)
        return base[sh] + sym['st_value']
    for rs in e.iter_sections():
        if rs['sh_type'] != 'SHT_REL': continue
        tgt = rs['sh_info']
        if tgt not in base: continue
        buf, B = img[tgt], base[tgt]
        for r in rs.iter_relocations():
            off, t = r['r_offset'], r['r_info_type']
            S = symaddr(syms[r['r_info_sym']])
            P = B + off
            if t in (47, 48):  # MOVW_ABS_NC / MOVT_ABS
                h1, h2 = struct.unpack_from('<HH', buf, off)
                A = _movw_get(h1, h2)
                if A & 0x8000: A -= 0x10000
                v = (S + A) & 0xFFFFFFFF
                v = v & 0xFFFF if t == 47 else v >> 16
                struct.pack_into('<HH', buf, off, *_movw_set(h1, h2, v))
            elif t in (10, 30):  # THM_CALL / THM_JUMP24
                h1, h2 = struct.unpack_from('<HH', buf, off)
                A = _bl_get(h1, h2)
                struct.pack_into('<HH', buf, off, *_bl_set(h1, h2, (S & ~1) + A - P))
            elif t == 2:  # ABS32
                A = struct.unpack_from('<I', buf, off)[0]
                struct.pack_into('<I', buf, off, (S + A) & 0xFFFFFFFF)
            else:
                raise NotImplementedError(f'reloc {t} in {rs.name}')
    funcs = {s.name: symaddr(s) & ~1 for s in syms if s['st_info']['type'] == 'STT_FUNC' and s['st_shndx'] != 'SHN_UNDEF'}
    return [(base[i], bytes(img[i])) for i in base], funcs
