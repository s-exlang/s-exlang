#define KNOB_IMPLEMENTATION
#include "knob.h"

// list -> '(' expression* ')'
// expression -> atom | list
// atom -> number | name | string | operator

typedef enum {
    ATOM_NULL,
    ATOM_NUM,
    ATOM_STR,
    ATOM_OP,
    ATOM_IDENT,
    ATOM_LIST,
    TYPE_COUNT
} Type;

static_assert(TYPE_COUNT == 6);
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
                    // KNOB_REALLOC(root->sub_atoms,sizeof(List));
                    // root->sub_atoms.count = 0;
                    // root->sub_atoms.capacity = 0;
                    // assert(root->sub_atoms != NULL);
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
int main(int argc,char** argv){
    int result = 0;
    Atom_t root = {0};
    root.type = TYPE_COUNT;
    Knob_String_Builder sb = {0};
    if(!knob_read_entire_file("./examples/add.gc",&sb)){
        knob_return_defer(false);
    }
    parse_text(sb.items,sb.count,&root);
    
    print_ast(&root,0);
    
defer:
    if(root.type == TYPE_COUNT){
        printf("No expression found in files...");
    }
    return result;
}