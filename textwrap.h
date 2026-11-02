#pragma once
// Splitting text into lines, without hardware (tested in test/test_wifi_text.cpp).

// How many characters of s fit in a line of maxChars, breaking at a space if
// possible. skip receives how much to advance in s to continue (it eats the break space).
inline int wrapNext(const char *s, int maxChars, int &skip) {
  int len = 0, lastSpace = -1;
  while (s[len] && len < maxChars) {
    if (s[len] == ' ') lastSpace = len;
    len++;
  }
  if (!s[len]) { skip = len; return len; }           // what remains fits whole
  if (s[len] == ' ') { skip = len + 1; return len; }  // break right at a space
  if (lastSpace > 0) { skip = lastSpace + 1; return lastSpace; }
  skip = len;  // word longer than the line: it is split
  return len;
}
