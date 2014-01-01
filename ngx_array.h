#ifndef _NGX_ARRAY_H_INCLUDED_
#define _NGX_ARRAY_H_INCLUDED_

struct ngx_array_s {
	void *elts;
	unsigned int nelts;
};

#endif
