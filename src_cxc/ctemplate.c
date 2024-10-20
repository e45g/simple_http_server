#include "%%NAME%%.h"
#include <stdlib.h>
#include <string.h>
%%PREPEND%%
char *%%FUNC_NAME%%(%%PROPS_NAME%% *props)
{
    char *output = calloc(%%RESPONSE_SIZE%%+1, sizeof(char));

    %%CODE%%

    return output;
}
