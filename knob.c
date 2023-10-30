#define KNOB_IMPLEMENTATION
#include "knob.h"

void copy_files_recursive(const char* path,char* path_base){
    Knob_File_Paths files = {0};
    char* path_needle = path_base != NULL ? path_base : "";
    char temp_path[260] = {0};
    if(!knob_read_entire_dir(path,&files)){
        assert(0 && "unreachable");
    }
    for(int i =0;i < files.count;++i){
        char* filename = files.items[i];
        if(strcmp(filename,".") == 0 || strcmp(filename,"..") == 0 ){
            continue;
        }
        int n_len = snprintf(temp_path,260,"%s/%s",path,filename);
        if(knob_get_file_type(temp_path) == KNOB_FILE_DIRECTORY){
            copy_files_recursive(temp_path,&temp_path[strlen(path)]);
        }
        int checkDir = 0;
        char* end = strstr(temp_path,path_needle);
        if(end == temp_path){
            end = filename;
        }
        else {
            end = &end[1];//Go forward one to avoid /
            checkDir = 1;
        }
        char end_path[260] = {0};
        if(checkDir){
            snprintf(end_path,260,"./Deployment%s",path_needle);
            knob_mkdir_if_not_exists(end_path);  
        }
        snprintf(end_path,260,"./Deployment/%s",end);
        knob_copy_file(temp_path,end_path);
    }
}
bool build_app(void)
{
    bool result = true;
    Knob_Cmd cmd = {0};
    Knob_Procs procs = {0};

    if (!knob_mkdir_if_not_exists("./Deployment")) {
        knob_return_defer(false);
    }

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
    char* dll_out_file = knob_temp_sprintf("./Deployment/fasmg%s",DLL_NAME);
    if(!knob_file_exists(dll_out_file)){
        knob_cmd_append(&cmd,exe_path,dll_src_path);
        knob_cmd_append(&cmd,dll_out_file);
        if (!knob_cmd_run_sync(cmd)) knob_return_defer(false);
    }
    cmd.count = 0;
    if(!knob_file_exists("./Deployment/x64.inc")){
        copy_files_recursive("./Libraries/fasmg/core/examples/x86/include",NULL);
    }



    #ifdef _WIN32
    // knob_cmd_append(&cmd,"set","LIB=\"C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Tools\\MSVC\\14.37.32822\\lib\\x86\"");
    // knob_cmd_run_sync(cmd);
    // cmd.count = 0;
    // knob_cmd_append(&cmd,"-L./Deployment","-lfasmg");//@TODO: can we create a .lib file to link the dll instead of loading it at runtime ?
    // knob_cmd_append(&cmd,"-DLDFLAGS=\"C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Tools\\MSVC\\14.37.32822\\lib\\x86\"");
    
    knob_cmd_append(&cmd, /*"./zig/zig.exe"*/"zig","cc");
    knob_cmd_append(&cmd,"-lkernel32","-lwinmm", "-lgdi32");
    knob_cmd_append(&cmd,"-D_CRT_SECURE_NO_WARNINGS");
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