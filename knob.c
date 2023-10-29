#define KNOB_IMPLEMENTATION
#include "knob.h"

bool build_app(void)
{
    bool result = true;
    Knob_Cmd cmd = {0};
    Knob_Procs procs = {0};

    if (!knob_mkdir_if_not_exists("./Deployment")) {
        knob_return_defer(false);
    }

    // fasmg ..\..\..\..\core\source\windows\dll\fasmg.asm fasmg.dll
    //compile fasmg
    char* extension = "";
    const char* base_fasm_path = "./Libraries/fasmg/core";
    char exe_path[260] = {0};
    char* dll_src_path;
    #ifdef _WIN32
        extension = ".exe";
        dll_src_path = knob_temp_sprintf("%s/source/windows/dll/fasmg.asm",base_fasm_path);
    #else
        dll_src_path = knob_temp_sprintf("%s/source/linux/x64/fasmg.asm",base_fasm_path);        
    #endif
    snprintf(exe_path,260,"%s/fasmg%s",base_fasm_path,extension);
    knob_cmd_append(&cmd,exe_path,dll_src_path);
    knob_cmd_append(&cmd,knob_temp_sprintf("./Deployment/fasmg%s",DLL_NAME));
    if (!knob_cmd_run_sync(cmd)) knob_return_defer(false);
    cmd.count = 0;


    #ifdef _WIN32
    knob_cmd_append(&cmd, /*"./zig/zig.exe"*/"zig","cc");
    knob_cmd_append(&cmd,"-lkernel32","-lwinmm", "-lgdi32");
    // knob_cmd_append(&cmd,"-L./Deployment","-lfasmg");//@TODO: can we create a .lib file to link the dll instead of loading it at runtime ?
    knob_cmd_append(&cmd,"-target","x86-windows");
    #elif __linux__
    knob_cmd_append(&cmd, "zig","cc");
    knob_cmd_append(&cmd,"-target","x86_64-linux");
    #endif
    knob_cmd_append(&cmd, "-static","-I./knob","-I./src");
    knob_cmd_append(&cmd, "--debug", "-std=c11", "-fno-sanitize=undefined","-fno-omit-frame-pointer");

    knob_cmd_append(&cmd,
        "./src/main.c",
    );
    knob_cmd_append(&cmd, "-o", knob_temp_sprintf("./Deployment/sx%s",extension));
    if (!knob_cmd_run_sync(cmd)) knob_return_defer(false);
    
defer:
    knob_cmd_free(cmd);
    knob_da_free(procs);
    return result;
}

int main( int argc, char** argv){
    if (!build_app()) return 1;
}