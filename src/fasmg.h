
   
/**
 * The `mem_region_t` structure:
 *
 * - It should be initially zeroed or may be filled with information about a block of memory allocated with VirtualAlloc.
 * - When the assembly is successful, the structure is filled with information about the size of the generated output and the address where it is stored.
 * - If the structure provided a memory region but it was too small to hold the output, a region is re-allocated with VirtualAlloc, releasing the previous one with VirtualFree. This way you can keep calling the function and have it re-use the same region of memory for output.
 * - You can also initialize the `output_addr` field to point to a region prepared with VirtualAlloc with MEM_RESERVE flag and set the initial size to zero. `fasmg_Assemble` should then be able to keep using the reserved address, committing as much memory as needed for the generated output.
 */
typedef struct mem_region {
    void* output_addr;  // a dd
    unsigned int size;  // a dd
} mem_region_t;


/**
 * @brief 
 * 
 * @return a pointer to the version string.
 */
const char* (*fasmg_GetVersion)(void);

/**
 * @brief Assembles the assembly. All arguments are optional - every one of them is allowed to be NULL.
 * @param source_string - Points to the text to be assembled directly from memory. Console versions of fasmg use this input channel for the instructions added from the command line with -I switch. It can also be used to assemble complete source not stored in any file.
 * @param source_path - The path to the source file. If both in-memory and file sources are provided, assembly proceeds on a combined text (the string from memory comes first).
 * @param output_region - A pointer to the memory region where the output is stored.
 * @param output_path - The path to the output file. When not provided, the output path is generated from the input path, similar to how the command-line fasmg does it. When neither path is present, the output is not written to a file.
 * @param stdout - A handle to a file or pipe that receives the messages generated with the DISPLAY command.
 * @param stderr - A handle to a file or pipe that receives the error messages. All the errors counted by the value returned from the function are reported with console-like messages here.
 *
 * @return - Returns the number of reported errors, with 0 indicating a successful assembly. If the returned value is negative, it indicates a fatal error, such as running out of memory or failing to write the output.
 */
int (*fasmg_Assemble)(char* source_string,char* source_path,mem_region_t* output_region,char* output_path, void* out,void* err);