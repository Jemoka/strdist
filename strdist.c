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
    char impossiblemask;
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
short cache[USHRT_MAX];

// Handle iteration
// TODO: BE SURE TO free() THE STRINGS GENERATED
// IN actions.* -> str ONCE YOU HAVE DONE USING
// THEM OTHERWISE YOU **WILL** DIE.

Actions
handle_iteration(Node *current, char *ref) {
    // Create the template for the bitmask
    // If possible, the bit is unset. If set,
    // the action is impossible.
    char mask = 0;

    /// Skipping ////
    // Create the new string
    char *tmp_skip = malloc(sizeof(char)*strlen(current -> str));
    // Just copy
    strlcpy(tmp_skip, current->str, strlen(current -> str));
    // And create the skip node: just move index ahead
    Node skip = { tmp_skip, current->indx + 1};
    // Check skip status; if skipping results in
    // something different, its unskippable
    if ((skip.str)[skip.indx-1] != ref[skip.indx-1])
        // Set unskippable mask
        mask |= UNSKIPPABLE;

    //// Appending ////
    // Get the append literal
    char to_concat = ref[current -> indx];
    // Create the new string
    char *tmp_append = malloc(sizeof(char)*strlen(current -> str))+1;
    // Copy the first half (up to, not including, the current index)
    strlcpy(tmp_append, current->str, current->indx);
    // Set the char
    tmp_append[current->indx] = to_concat;
    // Copy the second half
    strlcpy(tmp_append, (current->str)+(current->indx), strlen(current -> str)-(current->indx));
    // And create the append node and move index ahead
    Node append = { tmp_skip, current->indx + 1};

    //// Replacing ////
    // Create the new string
    char *tmp_replace = malloc(sizeof(char)*strlen(current -> str));
    // Get the replace literal
    char to_replace = ref[current -> indx];
    // Copy
    strlcpy(tmp_replace, current->str, strlen(current -> str));
    // Then, replace
    tmp_replace[current->indx] = to_replace;
    // And create the skip node and index ahead
    Node replace = { tmp_replace, current->indx + 1};
    // Check replace status; if replacing does nothing
    // then its considered unreplaceable
    if ((replace.str)[replace.indx-1] == ref[replace.indx-1])
        // Set unskippable mask
        mask |= UNSKIPPABLE;

    //// Ignoring ////
    // Create the new string
    char *tmp_ignore = malloc(sizeof(char)*strlen(current -> str))-1;
    // Copy the first half (up to, not including, the current index)
    strlcpy(tmp_ignore, current->str, current->indx);
    // Copy the second half, not including the current index
    strlcpy(tmp_ignore, (current->str)+(current->indx)+1, strlen(current -> str)-(current->indx)-1);
    // And create the ignore node and don't move index ahead
    Node ignore = { tmp_ignore, current->indx };
    // Checking of the current character is correct
    // Otherwise its not ignorable
    if ((ignore.str)[ignore.indx] != ref[ignore.indx])
        // Set unskippable mask
        mask |= UNIGNORABLE;

    return (Actions) {skip, append, replace, ignore, mask};
}

int main()
{
    /* Actions res = handle_iteration(); */
    /* printf("%s\n", res.skip.str); */
    
    return 0;
}

