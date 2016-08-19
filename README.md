# searchlol
search for onions (work in progress, toy implementation)

## What?
1. We take in a set of dictionaries, they should contain strings of valid base32 text (lowercase), we build these strings into a tree-like structure (lol look at me using computer sciencey words) and leave a flag on the branch of indicate a valid string terminates there.
2. We generate RSA keys, encode to ASN.1, SHA1 and base32 encode the first 10 bytes of the SHA1 output, giving us 16bytes of base32 characters (a through z and 2 through 7).
3. Starting at the root of our tree, we follow its branches, following the branch corresponding to each letter, essentially performing a parallel strcmp() of the onion address against all the words in our dictionary at once.
4. If the onion is a match, and we're not in "full" mode we save the onion. If we are in full mode, we recurse into search again from where the last word ended, only returning matching if the onion consists purely of strings in our dictionary.
