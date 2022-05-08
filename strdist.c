#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>

// Apparently min() is not a function in C
#define MIN(a,b) ((a) < (b) ? a : b)

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
hash_node(Node *n) {
    unsigned long str_hash = hash(n->str);

    return (str_hash + n->indx) % 32767;
}

// Handle iteration
// TODO: BE SURE TO free() THE STRINGS GENERATED
// IN actions.* -> str ONCE YOU HAVE DONE USING
// THEM OTHERWISE YOU **WILL** DIE.
Actions
handle_iteration(Node *current, char *ref) {
    // Sanity check first:
    // if we are done (current->str is ref)
    // then we have literally nothing to do.
    // Set the bitmask as 1111 (F) (all undoable)
    // then just move on
    if (strcmp(current->str,ref) == 0) {
        // Create empty action
        Actions result;
        // Set the mask to 0xF (1111) (ON ON ON ON)
        result.impossiblemask = 0xF;
        // Return!
        return result;
    }

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
    if ((skip.str)[skip.indx-1] != ref[skip.indx-1] || skip.indx > original_length)
        // Set unskippable mask
        mask |= UNSKIPPABLE;

    //// Appending ////
    // Get the append literal
    char to_concat = ref[current -> indx];
    // Create the new string
    char *tmp_append = malloc(sizeof(char)*original_length+1);
    // Copy the first half (up to, not including, the current index)
    strlcpy(tmp_append, current->str, current->indx+1);
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
        mask |= UNREPLACEABLE;

    //// Ignoring ////
    // Create the new string
    char *tmp_ignore = malloc(sizeof(char)*original_length);
    // Copy the first half (up to, not including, the current index)
    strlcpy(tmp_ignore, current->str, (current->indx)+1);
    // Copy the second half, not including the current index
    strlcpy(tmp_ignore+(current->indx), (current->str)+(current->indx)+1, original_length-(current->indx));
    // And create the ignore node and don't move index ahead
    Node ignore = { tmp_ignore, current->indx };
    // Checking of the current character is correct
    // Otherwise its not ignorable
    if ((ignore.str)[ignore.indx] != ref[ignore.indx] || ignore.indx >= original_length)
        // Set unskippable mask
        mask |= UNIGNORABLE;

    return (Actions) {skip, append, replace, ignore, mask};
}

// Function to destruct an action; specifically,
// all the strings pointing to it.
void
destruct(Actions *action) {
    free(action -> skip.str);
    free(action -> append.str);
    free(action -> ignore.str);
    free(action -> replace.str);
}

// Do the actual trace
// This function is recursive
void
do_trace(Node *node, short lvl, char *tgt, unsigned short *cache) {
    /* printf("'%s' indx=%d; %d; %d\n", node->str, node->indx, hash_node(node), lvl); */
    // Write into cache table
    // Hash the current node
    unsigned short hash = hash_node(node);
    // If we have not seen before, write lvl. If not
    // write the minimum of what we have and lvl. 
    if (cache[hash] == USHRT_MAX)
           cache[hash] = lvl;
    else {
        cache[hash] = MIN(cache[hash], lvl);
        // return because we have already gotten here
        return;
    }

    // Get next actions
    Actions next_actions = handle_iteration(node, tgt);

    // If we are done (i.e. mask=0b1111), we are done:
    if (next_actions.impossiblemask == 0xF) return;

    // For each doable next action, iterate!
    // If we can skip, calculate results of skip
    if (!(next_actions.impossiblemask & UNSKIPPABLE)) {
        Node next = next_actions.skip;
        /* printf("<skip>\n"); */
        do_trace(&next, lvl, tgt, cache);
        /* printf("</skip>\n"); */
    }

    // If we can append, calculate results of append
    if (!(next_actions.impossiblemask & UNAPPENDABLE)) {
        Node next = next_actions.append;
        printf("<append>\n");
        do_trace(&next, lvl+1, tgt, cache);
        printf("</append>\n");
    }

    // If we can ignore, calculate results of ignore
    if (!(next_actions.impossiblemask & UNIGNORABLE)) {
        Node next = next_actions.ignore;
        printf("<ignore>\n");
        do_trace(&next, lvl+1, tgt, cache); // ignoring does not change level
        printf("</ignore>\n");
    }

    // If we can replace, calculate results of replace
    if (!(next_actions.impossiblemask & UNREPLACEABLE)) {
        Node next = next_actions.replace;
        printf("<replace>\n");
        do_trace(&next, lvl+1, tgt, cache); 
        printf("</replace>\n");
    }

    // Free the next actions
    destruct(&next_actions);
}

// Find the minimum edit distance between two strings
unsigned short
strdist(char *src, char *tgt) {
    // Provide a stack cache, initialize it with
    // USHORT_MAX (infinitely far away)
    unsigned short cache[USHRT_MAX];
    for (int i=0; i<USHRT_MAX; i++) cache[i] = USHRT_MAX;

    // Declare an origin node and find the initial next actions
    Node origin_node = { src, 0 };

    // Fire away starting at level 0
    do_trace(&origin_node, 0, tgt, cache);

    // Declare the destination node, hash it
    Node destination_node = { tgt, strlen(tgt) };
    long hash = hash_node(&destination_node);
    printf("%d %d\n", hash, strlen(tgt));

    /* for (int i=0; i<USHRT_MAX; i++) */
    /*     if (cache[i] < 65535) */
    /*         printf("cache[%d] = %d\n", i, cache[i]); */

    // And finally, figure out the DP'd minimum
    // edit distance
    return cache[hash];
}

// Lookup solution


int main()
{
    // Allocate cache array on stack 
    printf("Distance: %d\n", strdist("what", "what1"));
    /* Node test = {"hewo", 2}; */
    /* Actions next = handle_iteration(&test, "hello"); */
    /* printf("'%s'\n", next.skip.str); */

    /* destruct(&next); */


    /* printf("%s; %ld\n", next.append.str, next.skip.indx); */
    /* printf("%s; %ld\n", next.ignore.str, next.skip.indx); */
    /* printf("%s; %ld\n", next.replace.str, next.skip.indx); */

    /* destruct(&next);     */
    return 0;
}

