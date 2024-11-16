void fast_strcat(char *dest, const char *src){
    while (*dest) dest++;
    while((*dest++ = *src++));
}
