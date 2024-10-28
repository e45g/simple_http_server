#ifndef layout
#define layout

typedef struct {
    
    char x[1024];

} LayoutProps;

char *render_layout(LayoutProps *props);

#endif
