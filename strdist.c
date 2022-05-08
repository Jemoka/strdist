#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>

// Struct to store structure of a node
typedef struct {
    char* str;
    size_t indx;
} Node;

enum ActionMask {
    UNSKIPPABLE = (1u << 0),
    UNAPPENDABLE = (1u << 1),
    UNREPLACEABLE = (1u << 2),
    UNIGNORABLE = (1u << 3),
};

// Struct to store three nodes representing next actions
typedef struct {
    // Results of skipping, appending, replacing, etc.
    Node skip;
    Node append;
    Node replace;
    Node ignore;

    // Bitmask (essentially 4 bools) to store whether or
    // not the action is possible. If possible, the bit
    // is unset. If set, the action is impossible.
    unsigned char impossiblemask : 4;
} Actions;

// Function to hash a string
// https://stackoverflow.com/questions/7666509/hash-function-for-string
unsigned long
hash(char *str)
{
    unsigned int hash = 5381;
    int c;

    while ((c = *str++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}

// Function to hash a node
// Uses the hash() function as a part of the expression
// We will take mod 32767 to cut it down to a `short`
unsigned short
node_hash(Node *n) {
    unsigned long str_hash = hash(n->str);

    return (str_hash + n->indx) % 32767;
}

// Allocate cache array on stack 
unsigned short cache[USHRT_MAX] = { USHRT_MAX };

// Handle iteration
// TODO: BE SURE TO free() THE STRINGS GENERATED
// IN actions.* -> str ONCE YOU HAVE DONE USING
// THEM OTHERWISE YOU **WILL** DIE.

Actions
handle_iteration(Node *current, char *ref) {
    // Create the template for the bitmask
    // If possible, the bit is unset. If set,
    // the action is impossible.
    unsigned char mask = 0;

    // Quick note about strlcpy: the l needs
    // one more than the length because the null
    // pointer.

    // Get original length
    size_t original_length = strlen(current -> str);
    
    /// Skipping ////
    // Create the new string
    char *tmp_skip = malloc(sizeof(char)*original_length);
    // Just copy
    strlcpy(tmp_skip, current->str, original_length+1);
    // And create the skip node: just move index ahead
    Node skip = { tmp_skip, current->indx + 1};
    // Check skip status; if skipping results in
    // something different, its unskippable
    if ((skip.str)[skip.indx-1] != ref[skip.indx-1] || skip.indx >= original_length)
        // Set unskippable mask
        mask |= UNSKIPPABLE;

    //// Appending ////
    // Get the append literal
    char to_concat = ref[current -> indx];
    // Create the new string
    char *tmp_append = malloc(sizeof(char)*original_length+1);
    // Copy the first half (up to, not including, the current index)
    strlcpy(tmp_append, current->str, current->indx);
    // Set the char
    tmp_append[current->indx] = to_concat;
    // Copy the second half
    strlcpy(tmp_append+(current->indx)+1, (current->str)+(current->indx), original_length-(current->indx)+1);
    // And create the append node and move index ahead
    Node append = { tmp_append, current->indx + 1};

    //// Replacing ////
    // Create the new string
    char *tmp_replace = malloc(sizeof(char)*original_length);
    // Get the replace literal
    char to_replace = ref[current -> indx];
    // Copy
    strlcpy(tmp_replace, current->str, original_length+1);
    // Then, replace
    tmp_replace[current->indx] = to_replace;
    // And create the skip node and index ahead
    Node replace = { tmp_replace, current->indx + 1};
    // Check replace status; if replacing does nothing
    // then its considered unreplaceable
    if ((replace.str)[replace.indx-1] == ref[replace.indx-1] || replace.indx >= original_length)
        // Set unskippable mask
        mask |= UNSKIPPABLE;

    //// Ignoring ////
    // Create the new string
    char *tmp_ignore = malloc(sizeof(char)*original_length)-1;
    // Copy the first half (up to, not including, the current index)
    strlcpy(tmp_ignore, current->str, current->indx+1);
    // Copy the second half, not including the current index
    strlcpy(tmp_ignore, (current->str)+(current->indx)+1, original_length-(current->indx));
    // And create the ignore node and don't move index ahead
    Node ignore = { tmp_ignore, current->indx };
    // Checking of the current character is correct
    // Otherwise its not ignorable
    if ((ignore.str)[ignore.indx] != ref[ignore.indx] || ignore.indx >= original_length)
        // Set unskippable mask
        mask |= UNIGNORABLE;

    return (Actions) {skip, append, replace, ignore, mask};
}

int main()
{
    char *src = "heresathing";
    char *tgt = "heraborathing";

    Node origin_node = { src, 0 };
    Actions next = handle_iteration(&origin_node, tgt);
    printf("%s\n", next.skip.str);
    printf("%s\n", next.append.str);
    printf("%s\n", next.ignore.str);
    printf("%s\n", next.replace.str);

    printf("hello---\n");
    free(next.skip.str);
    printf("bello---\n");
    free(next.append.str);
    printf("cello---\n");
    free(next.ignore.str);
    printf("hello---\n");
    free(next.replace.str);
    
    return 0;
}

