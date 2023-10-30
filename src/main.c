#define KNOB_IMPLEMENTATION
#include "knob.h"

#include "fasmg.h"

// list -> '(' expression* ')'
// expression -> atom | list
// atom -> number | name | string | operator

typedef enum {
    LINUX,
    WINDOWS,
    // BSDS,//Should we specify per BSD version or can we generalize this ?
    // MACOS,// It should be also ELF but maybe it's Mach-O ? Also do we want to support it ?
    TARGET_COUNT
} Target;

#ifdef __linux__//F security on Windows
static_assert(TARGET_COUNT == 2);
#endif
char* TARGET_FORMATS[] = {
    "ELF",
    "PE"
};

char* ARCH_REGS[2][64] = {
    //32bit
    {
        "eax",
        "ebx",
        "ecx"
    },
    //64bit
    {
        "rax",//Add more, we don't support 64 bit for now.
    }
};
#define GetRegName(config,index) ARCH_REGS[config.target_size > 64 ? 1 : 0 ][index]

typedef unsigned char u8;

typedef struct {
    Target target;
    u8 target_size;// 32 bit or 64 bit
    u8 isDebug;
} Config;

void make_base_target(Config* c){
    c->target_size = 32;//@TODO: when we support 64 bit, make it the default
    c->isDebug = 1;
    #ifdef __linux__
    c->target = LINUX;
    #elif defined(_WIN32)
    c->target = WINDOWS;
    #else
    assert(0 && "unreachable for now");
    #endif
}

typedef enum {
    ATOM_NULL,
    ATOM_NUM,
    ATOM_STR,
    ATOM_OP,
    ATOM_IDENT,
    ATOM_LIST,
    TYPE_COUNT
} Type;

#ifdef __linux__//F security on Windows
static_assert(TYPE_COUNT == 6);
#endif
char* TYPE_STRINGS[] = {
    "Null",
    "Number",
    "String",
    "Operator",
    "Identifier",
    "List"
};

typedef struct List List;
typedef struct Atom Atom_t;

struct List {
    Atom_t* items;
    size_t count;
    size_t capacity;
};

struct Atom {
    Type type;
    char* content;
    size_t c_len;
    Atom_t* par;
    List sub_atoms;
};

void parse_text(char* text,size_t len,Atom_t* root){
    int i =0;
    Atom_t* par = root;
    int is_new = 0;
    while(i < len){
        char c = text[i];
        switch (c){
            case'(':{
                Atom_t l = {0};
                l.type = ATOM_LIST;
                l.par = par;
                if(root->type == TYPE_COUNT){
                    root->type = ATOM_LIST;
                    memset(&root->sub_atoms,0,sizeof(List)); 
                }
                knob_da_append(&par->sub_atoms,l);
                par = &par->sub_atoms.items[par->sub_atoms.count-1];
                break;
            }
            case ')':{
                par = par->par;
                break;
            }
            case '+':{
                Atom_t op = {0};
                op.content = &text[i];
                op.type = ATOM_OP;
                op.c_len = 1;
                knob_da_append(&par->sub_atoms,op);
                break;
            }    
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':{
                Atom_t t = {0};
                Atom_t* num = &par->sub_atoms.items[par->sub_atoms.count-1];
                if(num->type == ATOM_NUM && !is_new){
                    num->c_len+=1;
                    break;
                }
                t.type = ATOM_NUM;
                t.c_len+=1;
                t.content = &text[i];
                knob_da_append(&par->sub_atoms,t);
                is_new = 0;
                break;
            }
            case ' ':{
                is_new = 1;
                break;
            }
            default:
                assert(0 && "Parsing: unreachable");
        }
        ++i;
    }
}

void print_ast(Atom_t* root,int level){
    char indents[64] = {0};
    char temp[64] = {0};
    for(int i = 0; i < level;++i){
        indents[i] = ' ';
    }
    if(root->c_len > 0){
        memcpy(temp,root->content,root->c_len);
    }
    knob_log(KNOB_INFO,"%sAtom of type: %s with content %s",indents,TYPE_STRINGS[root->type],temp);
    for(int i =0; i < root->sub_atoms.count;++i){
        print_ast(&root->sub_atoms.items[i],level+1);
    }
}
static Config config = {0};

void add_basic_funcs(Knob_String_Builder *out){

}
//@TODO: Should we remove this, it makes sense to learn but to make the compiler we can use what is provided by fasmg maybe ?
#define NUM_KFUNCS 2
void add_libraries(Knob_String_Builder *out){
    static char* kernel_funcs[] = {
        "exit",
        "write"
    };
    #ifdef _WIN32
    static char* real_kernel_funcs[] = {
        "ExitProcess",
        "WriteFile"
    };
    char* word_size = "dd";
    if(config.target_size > 32){
        word_size = "dq";
    }
    knob_sb_append_cstr(out,"section '.idata' import data readable writeable\n");
    knob_sb_append_cstr(out, "\tdd 0,0,0,RVA kernel_name,RVA kernel_table\n");
    knob_sb_append_cstr(out, "\tdd 0,0,0,0,0\n");
    // dd 0,0,0,RVA user_name,RVA user_table @TODO: Determine if we would need this;
    knob_sb_append_cstr(out,"\n\tkernel_table:\n");
    for(int i =0; i < NUM_KFUNCS; ++i){
        knob_sb_append_cstr(out,knob_temp_sprintf("\t\t%s %s RVA _%s\n",kernel_funcs[i],word_size,real_kernel_funcs[i]));
        knob_sb_append_cstr(out,knob_temp_sprintf("\t\t%s 0\n",word_size));
    }
    knob_sb_append_cstr(out, "\n\tkernel_name db 'KERNEL32.DLL',0\n");
    for(int i =0; i < NUM_KFUNCS; ++i){
        knob_sb_append_cstr(out,knob_temp_sprintf("\t\t_%s dw 0\n",real_kernel_funcs[i]));
        knob_sb_append_cstr(out,knob_temp_sprintf("\t\tdb '%s',0\n",real_kernel_funcs[i]));
    }
    if(config.target_size > 32){
        knob_sb_append_cstr(out,"section '.reloc' fixups data readable discardable	; needed for Win32s");
    }
    #else
    #endif
}

void gen_from_ast(Atom_t* root,Knob_String_Builder* out){
    if(out->count == 0){//generate header
        knob_sb_append_cstr(out,"include 'format/format.inc'\n\n");
        knob_sb_append_cstr(out,knob_temp_sprintf("format %s%s console\n\n",TARGET_FORMATS[config.target],config.target_size > 32 ? "64":""));
        knob_sb_append_cstr(out,"entry start\n");
        knob_sb_append_cstr(out,"start:\n");
        if(config.isDebug){
            // knob_sb_append_cstr(out,"\tint3\n");
        }

    }
    char* curr_op ="";
    int next_reg = 0;
    char* curr_reg = GetRegName(config,next_reg++);
    for(int i =0; i < root->sub_atoms.count;++i){//@TODO: Considering we may need to do operations lower down the Ast we may need to start from the end then back
        Atom_t curr = root->sub_atoms.items[i];
        if(curr.type == ATOM_OP){
            if(curr.content[0] == '+'){
                curr_op = "add";
                knob_sb_append_cstr(out,knob_temp_sprintf("\tmov\t%s,0\n",curr_reg));
            }
        }
        else{
            if(curr.type == ATOM_NUM){
                char num[16] = {0};
                memcpy(num,curr.content,curr.c_len);
                knob_sb_append_cstr(out,knob_temp_sprintf("\t%s\t%s,%s\n",curr_op,curr_reg,num));
            }
        }
    }
    knob_sb_append_null(out);
}
#define loadFunc(handle,funcname) funcname = dynlib_loadfunc(handle,#funcname)
int main(int argc,char** argv){
    make_base_target(&config);
    char* err_msg = "";
    int result = 0;
    char dll_path[260] ={0};
    snprintf(dll_path,260,".\\fasmg%s",DLL_NAME);
    void* dll_handle = dynlib_load(dll_path);
    if(!dll_handle){
        err_msg = "Assembler loading failed...";
        knob_return_defer(true);
    }
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wincompatible-function-pointer-types"
    loadFunc(dll_handle,fasmg_GetVersion);
    loadFunc(dll_handle,fasmg_Assemble);
#pragma GCC diagnostic pop
    if(!fasmg_GetVersion || !fasmg_Assemble){
        err_msg = "Assembler DLL loaded but could not load functions.";
        knob_return_defer(true);
    }
    char* fasm_version = fasmg_GetVersion();
    knob_log(KNOB_INFO,"This is the assembler version %s",fasm_version);

    Atom_t root = {0};
    root.type = TYPE_COUNT;
    Knob_String_Builder sb = {0};
    Knob_String_Builder out_sb = {0};
    if(!knob_read_entire_file("./examples/add.sx",&sb)){
        knob_return_defer(true);
    }
    parse_text(sb.items,sb.count,&root);
    mem_region_t output = {0};
    if(root.sub_atoms.count > 0){
        FILE* errs = fopen("errors.txt","wb+");
        gen_from_ast(&root.sub_atoms.items[0],&out_sb);
        add_libraries(&out_sb);
        int num_errs = fasmg_Assemble(out_sb.items,NULL,&output,NULL,errs,errs);
        fclose(errs);
        knob_write_entire_file("./out.txt",out_sb.items,out_sb.count);
        return 0;
        if(num_errs != 0){
            knob_log(KNOB_ERROR,"Failed to assemble code.");
            knob_return_defer(true);
        }
        else {
            if(!knob_write_entire_file("out.exe",output.output_addr,output.size)){
                err_msg = "Failed to write executable. Something must be wrong.";
                knob_return_defer(true);
            }
        }
    }
    // print_ast(&root,0);
    
defer:
    if(root.type == TYPE_COUNT){
        printf("No expression found in files...");
    }
    if(result){
        knob_log(KNOB_ERROR,"%s",err_msg);
    }
    dynlib_unload(dll_handle);
    fasmg_GetVersion = NULL;
    fasmg_Assemble = NULL;

    return result;
}