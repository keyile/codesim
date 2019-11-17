#!/usr/bin/env python3


from argparse import ArgumentParser
import random


def p(s):
    print(s, end='')


def print_tree(height, density):
    if height < 0:
        exit('Height must be greater than 0')
    if density < 0 or density > 1:
        exit('Density must be between 0.0 to 1.0')

    counter = 0
    def recursive_gen(depth):
        if depth > 0 and (random.random() > density):
            return

        nonlocal height
        nonlocal counter

        p('{')

        counter += 1
        p('{}'.format(counter))

        if depth < height:
            for i in range(0, 2):
                recursive_gen(depth + 1)

        p('}')

    recursive_gen(0)


def generate_test_case(height, density):
    print('[{')
    print('"testID":1,')
    
    p('"t1":"')
    print_tree(height, 1.0)
    p('",\n')

    p('"t2":"')
    print_tree(height, density)
    p('",\n')

    print('"d":0') # Diff doesn't matter because we just want a large test case for benchmark
    print('}]')


if __name__ == '__main__':
    parser = ArgumentParser()
    parser.add_argument('--height', type=int, required=True, help="Height of tree")
    parser.add_argument('--density', type=float, default=0.8, help="Density of tree")
    parser.add_argument('--seed', type=int, default=42, help="Seed for random generator to determine if a node and its children should be omitted")
    args = parser.parse_args()

    random.seed(args.seed)
    generate_test_case(args.height, args.density)
