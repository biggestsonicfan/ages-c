// Export decompiles for ages-decomp.
// args: <names.tsv> <roots.tsv> <outdir> <depth>
//   names.tsv: addr \t name   (applied to functions that still have a default name)
//   roots.tsv: addr \t name \t group \t file \t comment
// Each root goes to <outdir>/<group>/<file>.c; callees (up to depth, not other roots,
// not runtime/library code) to <outdir>/<group>/helpers/<name>.c. Writes <outdir>/<group>/helpers.tsv.
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.*;
import ghidra.program.model.symbol.SourceType;
import java.io.*;
import java.nio.file.*;
import java.util.*;

public class AgesExport extends GhidraScript {
    static final String SKIP = "^(i960|sysMem|sce|ADX|adx|CRS|CRI|cv|ROFS|SJ|mem|str|sprintf|printf|_|sqrtf|atan2?f?|sinf?|cosf?|floorf?|fabsf?|SQRT|abs|rand|srand|qsort)";
    DecompInterface di;
    String out;

    Function fn(Address a) throws Exception {
        Function f = getFunctionAt(a);
        if (f == null) {
            disassemble(a);
            f = createFunction(a, null);
        }
        return f;
    }

    void write(Function f, String path, String comment) throws Exception {
        DecompileResults r = di.decompileFunction(f, 180, monitor);
        String c = r.decompileCompleted() ? r.getDecompiledFunction().getC()
                                          : "/* decompile failed: " + r.getErrorMessage() + " */\n";
        File file = new File(out, path + ".c");
        file.getParentFile().mkdirs();
        try (PrintWriter w = new PrintWriter(file)) {
            w.print("/* " + comment + " */\n");
            w.print(c.replace("\r\n", "\n"));
        }
    }

    void applyNames(String path) throws Exception {
        for (String l : Files.readAllLines(Paths.get(path))) {
            String[] p = l.split("\t");
            Function f = getFunctionAt(toAddr(Long.parseLong(p[0], 16)));
            if (f != null && f.getName().startsWith("FUN_"))
                f.setName(p[1], SourceType.USER_DEFINED);
        }
    }

    List<String[]> loadRoots(String path, Set<Address> rootAddrs) throws Exception {
        List<String[]> roots = new ArrayList<>();
        for (String l : Files.readAllLines(Paths.get(path))) {
            String[] p = l.split("\t", -1);
            roots.add(p);
            Address ad = toAddr(Long.parseLong(p[0], 16));
            rootAddrs.add(ad);
            Function f = fn(ad);
            if (f != null && f.getName().startsWith("FUN_") && !p[1].isEmpty())
                f.setName(p[1], SourceType.USER_DEFINED);
        }
        return roots;
    }

    static List<Function> sorted(Set<Function> fs) {
        List<Function> l = new ArrayList<>(fs);
        l.sort(Comparator.comparing(Function::getEntryPoint));
        return l;
    }

    boolean skip(Function c, Set<Address> rootAddrs, Set<Address> seen) {
        Address ca = c.getEntryPoint();
        return c.isThunk() || c.isExternal() || rootAddrs.contains(ca) || seen.contains(ca)
            || c.getName().matches(SKIP + ".*");
    }

    // Write the callees of f down to depth into <group>/helpers/, listing each in w.
    void exportHelpers(Function f, String[] p, int depth, Set<Address> rootAddrs, Set<Address> seen,
                       PrintWriter w) throws Exception {
        List<Function> level = List.of(f);
        for (int d = 1; d <= depth; d++) {
            List<Function> next = new ArrayList<>();
            for (Function g : level)
                for (Function c : sorted(g.getCalledFunctions(monitor))) {
                    if (skip(c, rootAddrs, seen)) continue;
                    Address ca = c.getEntryPoint();
                    seen.add(ca);
                    String name = c.getName().startsWith("FUN_") ? c.getName() : c.getName() + "_" + ca;
                    write(c, p[2] + "/helpers/" + name, "helper " + c.getName() + " @ " + ca
                          + ", called (depth " + d + ") from " + p[1]);
                    w.printf("%s\t%s\t%d\t%s\t%d%n", ca, c.getName(), c.getBody().getNumAddresses(), p[1], d);
                    next.add(c);
                }
            level = next;
        }
    }

    @Override
    public void run() throws Exception {
        String[] a = getScriptArgs();
        out = a[2];
        int depth = Integer.parseInt(a[3]);
        applyNames(a[0]);
        Set<Address> rootAddrs = new HashSet<>();
        List<String[]> roots = loadRoots(a[1], rootAddrs);
        di = new DecompInterface();
        di.openProgram(currentProgram);
        Map<String, Set<Address>> done = new HashMap<>();
        Map<String, PrintWriter> idx = new HashMap<>();
        for (String[] p : roots) {
            Function f = fn(toAddr(Long.parseLong(p[0], 16)));
            if (f == null) { println("no function at " + p[0]); continue; }
            write(f, p[2] + "/" + p[3], p[4]);
            PrintWriter w = idx.computeIfAbsent(p[2], k -> {
                try { return new PrintWriter(new File(out, k + "/helpers.tsv")); }
                catch (IOException e) { throw new RuntimeException(e); }
            });
            exportHelpers(f, p, depth, rootAddrs, done.computeIfAbsent(p[2], k -> new HashSet<>()), w);
        }
        for (PrintWriter w : idx.values()) w.close();
        di.dispose();
    }
}
