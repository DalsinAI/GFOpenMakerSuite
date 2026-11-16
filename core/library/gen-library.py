#!/usr/bin/env python3
"""The OpenMake libraries' glue, written from each library's .sfd.

For library/<name>_lib.sfd beside an app's include/ this writes:
  include/inline/<name>.h   the calls for a program: each a jsr through the
                            base in a6, its arguments in the .sfd's registers
                            (OM_LIB_HOST: through om_host_vector instead, so
                            the host test runs the same path)
  include/proto/<name>.h    the base and the inline calls
  library/<name>_vectors.h  the library's side: each call taking its
                            registers and calling the C function, and
                            <NAME>_VECTORS for the jump table
  library/<name>_link.c     the C API through the library, for a program
                            to link instead of the sources

The C function is the call's name made snake case: OM_ClampSpeed is
om_clamp_speed. A float travels as its bits in a data register (a float
in a register means the FPU's on one machine and nothing on another).

  gen-library.py machines/library/openmakemachine_lib.sfd [--check]
--check says whether the files are as it would write them.
MIT, Copyright (c) 2026 AmiCradle Team."""
import re
import sys
from pathlib import Path

SCRATCH = ["d0", "d1", "a0", "a1"]


def snake(name):
    pre, rest = name.split("_", 1)
    return pre.lower() + "_" + re.sub(r"(?<!^)([A-Z])", r"_\1", rest).lower()


def parse(path):
    lib = {"calls": [], "includes": []}
    bias = None
    for raw in path.read_text().splitlines():
        line = raw.strip()
        if not line or line.startswith("*"):
            continue
        if line.startswith("=="):
            key, _, val = line[2:].partition(" ")
            if key == "base":
                lib["base"] = val.strip().lstrip("_")
            elif key == "libname":
                lib["libname"] = val.strip()
            elif key == "bias":
                bias = int(val)
            elif key == "include":
                lib["includes"].append(val.strip())
            continue
        m = re.match(r"^(.+?)\s*\b(\w+)\((.*)\)\((.*)\)$", line)
        if not m:
            sys.exit(f"{path}: cannot read: {line}")
        ret, name, args, regs = m.groups()
        args = [a.strip() for a in args.split(",") if a.strip() and a.strip() != "void"]
        regs = [r.strip() for r in regs.split(",") if r.strip()]
        if len(args) != len(regs):
            sys.exit(f"{path}: {name}: {len(args)} arguments in {len(regs)} registers")
        params = []
        for a, r in zip(args, regs):
            am = re.match(r"^(.*?)(\w+)$", a)
            params.append({"type": am.group(1).strip(), "name": am.group(2), "reg": r})
        lib["calls"].append({"ret": ret.strip(), "name": name, "c": snake(name), "params": params, "bias": bias})
        bias += 6
    return lib


def wire(t):
    """The type a value has in a register."""
    return "ULONG" if t == "float" else t


def decl(t, n):
    return f"{t} *{n}" if False else (f"{t}{n}" if t.endswith("*") else f"{t} {n}")


def inline_h(lib, stem):
    g = f"INLINE_{stem.upper()}_H"
    base = lib["base"]
    out = [f"/* Written by core/library/gen-library.py from {stem}_lib.sfd: do not edit. */",
           f"#ifndef {g}", f"#define {g}", "", '#include "omlib.h"']
    out += [f"#include {i}" for i in lib["includes"] if "exec/types" not in i]
    out += ["", f"extern struct Library *{base};", ""]
    for c in lib["calls"]:
        ps = c["params"]
        sig = ", ".join(decl(p["type"], p["name"]) for p in ps) or "void"
        out.append(f"static __inline__ {c['ret']} {c['name']}({sig})")
        out.append("{")
        out.append("#ifdef OM_LIB_HOST")
        types = ", ".join([wire(p["type"]) for p in ps] + ["struct Library *"])
        vals = ", ".join([f"om_lib_bits({p['name']})" if p["type"] == "float" else p["name"] for p in ps] + [base])
        call = f"(({wire(c['ret'])} (*)({types}))om_host_vector({base}, {c['bias']}))({vals})"
        if c["ret"] == "void":
            out.append(f"    {call};")
        elif c["ret"] == "float":
            out.append(f"    return om_lib_float({call});")
        else:
            out.append(f"    return {call};")
        out.append("#else")
        used = {p["reg"] for p in ps}
        out.append(f"    register struct Library *_a6 __asm(\"a6\") = {base};")
        if "d0" not in used:
            out.append("    register ULONG _d0 __asm(\"d0\");")
        for p in ps:
            v = f"om_lib_bits({p['name']})" if p["type"] == "float" else f"(ULONG)({p['name']})"
            out.append(f"    register ULONG _{p['reg']} __asm(\"{p['reg']}\") = {v};")
        outs = ['"+r"(_d0)' if "d0" in used else '"=r"(_d0)']
        outs += [f'"+r"(_{r})' for r in SCRATCH[1:] if r in used]
        ins = ['"r"(_a6)'] + [f'"r"(_{p["reg"]})' for p in ps if p["reg"] not in SCRATCH]
        clob = [f'"{r}"' for r in SCRATCH if r not in used and r != "d0"] + ['"fp0"', '"fp1"', '"cc"', '"memory"']
        out.append(f"    __asm__ volatile(\"jsr a6@(-{c['bias']}:W)\" : {', '.join(outs)} : {', '.join(ins)} : {', '.join(clob)});")
        if c["ret"] == "float":
            out.append("    return om_lib_float(_d0);")
        elif c["ret"] != "void":
            out.append(f"    return ({c['ret']})_d0;")
        out.append("#endif")
        out.append("}")
        out.append("")
    out.append("#endif")
    return "\n".join(out) + "\n"


def proto_h(lib, stem):
    g = f"PROTO_{stem.upper()}_H"
    return "\n".join([f"/* Written by core/library/gen-library.py from {stem}_lib.sfd: do not edit. */",
                      f"#ifndef {g}", f"#define {g}", "",
                      f"#define {stem.upper()}_NAME \"{lib['libname']}\"",
                      f"#include <inline/{stem}.h>", "", "#endif"]) + "\n"


def vectors_h(lib, stem):
    out = [f"/* Written by core/library/gen-library.py from {stem}_lib.sfd: do not edit.",
           " * The library's side of each call: its registers to the C function. */",
           '#include "omlib.h"']
    out += [f"#include {i}" for i in lib["includes"] if "exec/types" not in i]
    out.append("")
    names = []
    for c in lib["calls"]:
        ps = c["params"]
        regs = ", ".join([f"REG({p['reg']}, {decl(wire(p['type']), p['name'])})" for p in ps] +
                         ["REG(a6, struct Library *base)"])
        fn = f"LIB_{c['name']}"
        names.append(fn)
        args = ", ".join(f"om_lib_float({p['name']})" if p["type"] == "float" else p["name"] for p in ps)
        out.append(f"static {wire(c['ret'])} {fn}({regs})")
        out.append("{")
        out.append("    (void)base;")
        if c["ret"] == "void":
            out.append(f"    {c['c']}({args});")
        elif c["ret"] == "float":
            out.append(f"    return om_lib_bits({c['c']}({args}));")
        else:
            out.append(f"    return {c['c']}({args});")
        out.append("}")
        out.append("")
    out.append(f"#define {stem.upper()}_LAST_BIAS {lib['calls'][-1]['bias']}")
    out.append(f"#define {stem.upper()}_VECTORS \\")
    out.append("    " + ", \\\n    ".join(f"(APTR){n}" for n in names))
    return "\n".join(out) + "\n"


def link_c(lib, stem):
    out = [f"/* Written by core/library/gen-library.py from {stem}_lib.sfd: do not edit.",
           f" * The C API through {lib['libname']}: a program links this instead of",
           f" * the sources, and opens {lib['base']} itself. */",
           f"#include <proto/{stem}.h>", ""]
    for c in lib["calls"]:
        ps = c["params"]
        sig = ", ".join(decl(p["type"], p["name"]) for p in ps) or "void"
        args = ", ".join(p["name"] for p in ps)
        out.append(f"{c['ret']} {c['c']}({sig})")
        out.append("{")
        out.append(f"    {'' if c['ret'] == 'void' else 'return '}{c['name']}({args});")
        out.append("}")
        out.append("")
    return "\n".join(out).rstrip() + "\n"


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    check = "--check" in sys.argv
    bad = 0
    for a in args:
        sfd = Path(a)
        stem = sfd.name[: -len("_lib.sfd")]
        lib = parse(sfd)
        app = sfd.parent.parent
        files = {app / "include" / "inline" / f"{stem}.h": inline_h(lib, stem),
                 app / "include" / "proto" / f"{stem}.h": proto_h(lib, stem),
                 sfd.parent / f"{stem}_vectors.h": vectors_h(lib, stem),
                 sfd.parent / f"{stem}_link.c": link_c(lib, stem)}
        for path, text in files.items():
            if check:
                if not path.exists() or path.read_text() != text:
                    print(f"{path} is not as {sfd} writes it")
                    bad = 1
            else:
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_text(text)
    sys.exit(bad)


if __name__ == "__main__":
    main()
