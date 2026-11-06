import json, sys, itertools, numpy as np
sys.path.insert(0, '..')
import make_reference as R
F = json.load(open('features.json'))
meas = {'theirs_base': R.ORIGINAL[1]}
for (bx, by), t in R.DSE.items(): meas[f'theirs_rect_{bx}_{by}'] = t[1]
keys = sorted(meas); y = np.array([meas[k] * 240e3 for k in keys])
feats = ['ins', 'bytes', 'wide', 'ld', 'st', 'br_taken']
def val(f, n): return f['ld_ram'] + f['ld_flash'] if n == 'ld' else f[n]
best = []
for r in (1, 2, 3):
    for combo in itertools.combinations(feats, r):
        X = np.array([[val(F[k], n) for n in combo] for k in keys], float)
        w = np.linalg.lstsq(X, y, rcond=None)[0]
        if (w < 0).any(): continue
        loo = []
        for i in range(len(keys)):
            m = np.arange(len(keys)) != i
            wi = np.linalg.lstsq(X[m], y[m], rcond=None)[0]; loo.append(abs(X[i] @ wi - y[i]) / y[i])
        best.append((max(loo), np.mean(loo), combo, w))
best.sort(key=lambda b: b[0])
for b in best[:8]:
    print('LOO max %5.1f%% mean %4.1f%%  %s  %s' % (100*b[0], 100*b[1], b[2], np.round(b[3], 3)))
print()
models = best[:3]
res = {}
for k in sorted(F):
    if not k.startswith('ours_'): continue
    ps = [sum(w_ * val(F[k], n) for n, w_ in zip(c, w)) / 240 for _, _, c, w in models]
    res[k[5:]] = (min(ps), float(np.median(ps)), max(ps))
    print(f'{k[5:]:20s} ' + '  '.join(f'{p:7.1f}' for p in ps) + f'   -> {min(ps):.0f}-{max(ps):.0f} us')
# also how the same models do on their measured points
print()
for k in keys:
    ps = [sum(w_ * val(F[k], n) for n, w_ in zip(c, w)) / 240 for _, _, c, w in models]
    print(f'{k:20s} meas {meas[k]*1000:7.1f}  model ' + ' '.join(f'{p:7.1f}' for p in ps))
json.dump({'models': [[list(c), list(w), m, a] for m, a, c, w in models], 'ours_us': res}, open('pred.json', 'w'), indent=1)
