// Writes <out>/<prog>/functions.tsv (entry, name, size, callees) and, with arg "decomp", one .c per function.
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.program.model.listing.*;
import java.io.*;
import java.util.*;

public class ExportFuncs extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] a = getScriptArgs();
        File out = new File(a[0], currentProgram.getName().replaceAll("[^A-Za-z0-9_.]", "_"));
        boolean dec = a.length > 1 && a[1].equals("decomp");
        new File(out, "c").mkdirs();
        DecompInterface di = new DecompInterface();
        if (dec) di.openProgram(currentProgram);
        try (PrintWriter tsv = new PrintWriter(new File(out, "functions.tsv"))) {
            for (Function f : currentProgram.getFunctionManager().getFunctions(true)) {
                if (monitor.isCancelled()) break;
                StringBuilder cs = new StringBuilder();
                for (Function c : f.getCalledFunctions(monitor))
                    cs.append(c.getEntryPoint()).append(',');
                tsv.printf("%s\t%s\t%d\t%s%n", f.getEntryPoint(), f.getName(),
                    f.getBody().getNumAddresses(), cs);
                if (!dec) continue;
                DecompileResults r = di.decompileFunction(f, 120, monitor);
                String c = r.decompileCompleted() ? r.getDecompiledFunction().getC()
                                                  : "/* decompile failed: " + r.getErrorMessage() + " */\n";
                try (PrintWriter w = new PrintWriter(new File(out, "c/" + f.getEntryPoint() + "_" + f.getName() + ".c"))) {
                    w.print(c);
                }
            }
        }
        di.dispose();
    }
}
