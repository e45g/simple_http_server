#include "layout.h"
#include <stdlib.h>
#include <string.h>

#include <stdio.h>


char *render_layout(LayoutProps *props)
{
    char *output = calloc(319+1, sizeof(char));

    	strcat(output, "<!DOCTYPE html><html lang=\"en\">    <head>        <meta charset=\"UTF-8\">        <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">        <title>Document</title>    </head>    <body>    <h1>");
	props->x[0] = 'q';
	strcat(output, "</h1>    ");
	

     
	strcat(output, "    <p>");
	strcat(output, props->x);


    return output;
}
