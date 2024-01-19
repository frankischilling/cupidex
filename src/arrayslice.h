/* 
 * The type ArraySlice is used to make it easier to refer to "sections" of
 * arrays. It should be used alongside the array, my recommendation is to have
 * the slice before the array, having the same name as the array except for a
 * "_slice" in the end.
 *
 * This implementation of ArraySlice is licensed under the public domain and
 * all further modifications are also licensed under the public domain except
 * if explicitly noted.
 */

// start is part of the slice, but end isn't
typedef struct {
	size_t start;
	size_t end;
} ArraySlice;

#define ArraySlice_LEN(as) ((as).end - (as).start)
#define ArraySlice_NTH(as, arr, n) (arr[(as).start + (n)])


