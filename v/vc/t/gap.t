#!/usr/bin/env jtest

$ vc '[] gap ...'
1 >= '()'

%

$ vc '[123] gap ...'
1 >= '[]'

%

$ vc '0 .. 3 gap ...'
1 >= '[0, 1, 1, 2, 2, 3]'

%

$ vc '0 .. 7 gap Math.product'
1 >= '[0, 2, 6, 12, 20, 30, 42]'

%

$ vc '0 .. 5 map { v^2 } gap {b - a}'
1 >= '[1, 3, 5, 7, 9]'

%

$ vc '[2, 4, 0, -12, -5] gap {b - a}'
1 >= '[2, -4, -12, 7]'