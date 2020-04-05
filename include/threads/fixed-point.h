#define F (1 << 14)
#define INT_MAX ((1<<31) - 1)
#define INT_MIN (-(1 << 31))
/* x and y is fixed point numbers in 17.14 format
 * n is integer */

int int_to_fp(int n);           /* convert integer into fixed point */
int fp_to_int_rounddown(int x); /* convert fixed point into integer (round down) */
int fp_to_int_round(int x);     /* convert fixed point into integer (round) */
int add_fp(int x, int y);       /* Addition of two fixed point number */
int sub_fp(int x, int y);       /* Subtraction of two fixed point number */
int add_fp_int(int x, int n);   /* Addition of fixed point and integer */
int sub_fp_int(int x, int n);   /* Subtraction between fixed point and integer */
int mult_fp(int x, int y);      /* Multiplication of two fixed point number */
int mult_fp_int(int x, int n);  /* Multiplication of fixed point and integer */
int div_fp(int x, int y);       /* Division between two fixed point number */
int div_fp_int(int x, int n);   /* Division betwwen fixed point and integer */

int int_to_fp (int n)
{
	return n * F;
}

int fp_to_int_rounddown (int x)
{
	return x / F;
}

int fp_to_int_round (int x)
{
	if (x >= 0)
		return (x + F / 2) / F;
	else
		return (x - F / 2) / F;
}

int add_fp (int x, int y)
{
	return x + y;
}

int sub_fp (int x, int y)
{
	return x - y;
}

int add_fp_int (int x, int n)
{
	return x + n * F;
}

int sub_fp_int (int x, int n)
{
	return x - n * F;
}

int mult_fp (int x, int y)
{
	return (int) (((int64_t) x) * y / F);
}

int mult_fp_int (int x, int n)
{
	return x * n;
}

int div_fp (int x, int y)
{
	return (int) (((int64_t) x) * F / y);
}

int div_fp_int (int x, int n)
{
	return x / n;
}
