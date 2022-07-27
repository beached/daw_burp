DAW Binary User Reflection Protocol (B.U.R.P) is a library to help read and write data structures.  Writing can be done to any type that is a Writeable Output Type(char/byte like buffers, span<char/byte> like types, strings, files, ostream's...  Reading can be done from Readable Input Types of similar types.

For reflection, Boost.Describe is used along with a fallback to customization points.
