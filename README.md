# c-tools

Those are the projects I've done as a part of low-level programming classes

## 1-ini-parser

`gcc ini-parser.c -o ini-parser`

`./ini-parser <file> {<query>|lint}`

### Help

```
❯ ./ini-parser help
Exactly two positional arguments are required:
1. source file
2. query/flag "lint"

Single query either:
1) fetches multiple variables separated by whitespace
2) performs operations from "+-*/" set

IMPORTANT: all operations are conducted sequentially
(in contradiction to the laws of mathematics)

Maximal supported line length is 1024 characters
Maximal output size is 1024 characters
```

### Multiple variable lookup

```
❯ ./ini-parser small.ini scholarly-collection.shallow-a
996698

❯ ./ini-parser small.ini pristine-wish.youthful-east
QrfhHfwUrYXeKevENYkR

❯ ./ini-parser small.ini 'double-piece.faint-affair glistening-working.agile-wheel'
714771
OpCtMOsHTmwPowOdXaZbEKUv
```

### Expression evaluation

\*with nuances mentioned in help

```
❯ ./ini-parser small.ini 'scholarly-collection.shallow-a / scholarly-collection.shallow-a + double-piece.faint-affair'
714772.000000

❯ ./ini-parser small.ini 'pristine-wish.youthful-east + glistening-working.agile-wheel + slippery-thing.moral-ordinary'
QrfhHfwUrYXeKevENYkROpCtMOsHTmwPowOdXaZbEKUvfDulyrtjUHnKPwwAZ
```

### Linting

Prints bad lines

```
❯ ./ini-parser corrupted.ini lint
105: [dizzy!!!-academic-excellent-some-ideal-serve]
129: ashamed-firm-last-extra-small-untrue-fold]
130: double-celebrated-some-scented-bogus-diet wUQOOtOtqnZJMnmRwLtnkWgkgMRZAuHsXrBEGtcXWqQQRVcNvQ
153: [ordinary-unknown-exotic-utilized-worrisome-=fuel]
164: memorable-made-up-thick-fixed-acidic-chicken]
178: wobbly-delaye#d-feline-vast-well-groomed-wing = tRLfbEMlETGjyJZloyDjjCPzOdvxutYhV
565: [paltry-similar-probab*fatherly-adolescent-specialist]
758: fumbling-lavish-upse^-gummy-safe-straight-factor
763: ahfd$$$dorable-sweaty-violent-serious-junior-ad = 803399
```

## 2-bmp-steganography

`gcc bmp-steganography.c -o bmp-steganography`

`./bmp-steganography <input> [<output>] [<text-to-encode>]`

## 3-bmp-generator

`gcc mandelbrot.c -o mandelbrot -lm`
