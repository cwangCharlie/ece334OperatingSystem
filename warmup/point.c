#include <assert.h>
#include "common.h"
#include "point.h"
#include <math.h>
void
point_translate(struct point *p, double x, double y)
{
	p->x += x;
	p->y += y;
}

double
point_distance(const struct point *p1, const struct point *p2)
{
	double calcuateX = p2->x - p1->x;
	double calcuateY = p2->y - p1->y;

	

	return sqrt(pow(calcuateX,2.0)+pow(calcuateY,2.0));
}

int
point_compare(const struct point *p1, const struct point *p2)
{
	double p1D = sqrt(pow(p1->x,2.0)+pow(p1->y,2.0));
	double p2D = sqrt(pow(p2->x,2.0)+pow(p2->y,2.0));
	
	int result;
	if(p1D>p2D){
		result = 1;
	}else if(p1D<p2D){
		result = -1;

	}else if(p1D ==p2D){
		result = 0;

	}

	return result;
}
