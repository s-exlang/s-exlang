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
    knob_cmd_append(&cmd, "./zig/zig.exe","cc");
    knob_cmd_append(&cmd, "-static");
    knob_cmd_append(&cmd, "--debug", "-std=c11", "-fno-sanitize=undefined","-fno-omit-frame-pointer");
    knob_cmd_append(&cmd,"-I./knob","-lkernel32","-lwinmm", "-lgdi32");
    knob_cmd_append(&cmd,"-target","x86_64-windows");
    knob_cmd_append(&cmd,
        "./src/main.c",
    );
    knob_cmd_append(&cmd, "-o", "./Deployment/sx.exe");
    if (!knob_cmd_run_sync(cmd)) knob_return_defer(false);
    
defer:
    knob_cmd_free(cmd);
    knob_da_free(procs);
    return result;
}

int main( int argc, char** argv){
    if (!build_app()) return 1;
}