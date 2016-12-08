# InstrusiveSortedDeque
A wrapper around std::deque providing O(log(n)) deletions at arbitrary locations

## Main properties
- Intrusive: The values provide a key, by which they can be searched, and methods for marking them as deleted and for check whether they are deleted.
- Sorted: The values are stored in ascending order of the key.
- The front and back elements are always non-deleted.

## Building Requirements
This is a header only class, so it can only be used as part of a project. The build environment should provide the following:
- C++ 14 compiler
- boost iterator library

## Example use-case
The class was designed to meet the following use-case:

- Values are inserted from the back.
- The size of individual values is a few tens of bytes.
- Values are naturally ordered in ascending order by one of their fields, which servers as a unique identifier.
- However, it is possible, although extremely rare, for a value's key to be not greater thatn the one at the back.
- Values are often removed from the front, but can also be removed from arbitrary locations specified by the key, so look-up and removal should be fast.
- Copying a contiguous subset of the values to a new data-structure of this type should be cheap.
- Clearing should be cheap.
- Typically contains tens or hundreds of values, but thousands are also possible.
- Performance should be consistent, as I'm using this for a soft real-time system.
