#include "utils.h"

bool is_prime(unsigned int n)
{
    for (int i = 2; i <= n / 2; i++)
    {
        if (n % i == 0)
        {
            return false;
        }
    }

    return n <= 1 ? false : true;
}

unsigned int find_next_prime(unsigned int n)
{
    while (!is_prime(n))
    {
        n++;
    }

    return n;
}
