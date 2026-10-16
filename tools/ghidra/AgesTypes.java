// Give the i960 runtime helpers prototypes and the register file a struct, so the
// decompiler shows each call's arguments. args: <names.tsv> (applied first, as in AgesExport)
import ghidra.app.script.GhidraScript;
import ghidra.app.util.cparser.C.CParser;
import ghidra.app.cmd.function.ApplyFunctionSignatureCmd;
import ghidra.program.model.data.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.symbol.SourceType;
import java.nio.file.*;
import java.util.*;

public class AgesTypes extends GhidraScript {
    static final String STRUCT =
        "struct i960_regs { unsigned int r[16]; unsigned int g[16]; unsigned char pad[0xc0];"
        + " float fp[4]; unsigned int ip; unsigned int ac; unsigned int pc; };";
    static final String[] PROTOS = {
        "struct i960_regs *i960GetRegs(void);",
        "unsigned int *i960GetRegP(unsigned int reg);",
        "void i960SetReg(unsigned int reg, unsigned int value);",
        "void i960SetIPAddr(unsigned int addr);",
        "void i960ReturnProc(void);",
        "void i960Ret(void);",
        "void sysMemReadB(void *dst, unsigned int addr);",
        "void sysMemReadS(void *dst, unsigned int addr);",
        "void sysMemReadW(void *dst, unsigned int addr);",
        "void sysMemReadL(void *dst, unsigned int addr);",
        "void sysMemReadT(void *dst, unsigned int addr);",
        "void sysMemReadQ(void *dst, unsigned int addr);",
        "void sysMemWriteB(unsigned int addr, void *src);",
        "void sysMemWriteS(unsigned int addr, void *src);",
        "void sysMemWriteW(unsigned int addr, void *src);",
        "void sysMemWriteL(unsigned int addr, void *src);",
        "void sysMemWriteT(unsigned int addr, void *src);",
        "void sysMemWriteQ(unsigned int addr, void *src);",
        "void sysCopWriteFifo(unsigned int *src, int count);",
        "void sysCopReadFifo(unsigned int *dst, int count);",
    };

    @Override
    public void run() throws Exception {
        String[] a = getScriptArgs();
        if (a.length > 0)
            for (String l : Files.readAllLines(Paths.get(a[0]))) {
                String[] p = l.split("\t");
                Function f = getFunctionAt(toAddr(Long.parseLong(p[0], 16)));
                if (f != null && f.getName().startsWith("FUN_"))
                    f.setName(p[1], SourceType.USER_DEFINED);
            }
        DataTypeManager dtm = currentProgram.getDataTypeManager();
        CParser cp = new CParser(dtm, true, null);
        cp.parse(STRUCT);
        for (String s : PROTOS) {
            FunctionDefinition fd = (FunctionDefinition) cp.parse(s);
            List<Function> fs = getGlobalFunctions(fd.getName());
            if (fs.isEmpty()) { println("not found: " + fd.getName()); continue; }
            for (Function f : fs)
                new ApplyFunctionSignatureCmd(f.getEntryPoint(), fd, SourceType.USER_DEFINED).applyTo(currentProgram);
        }
    }
}
