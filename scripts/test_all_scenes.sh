#!/bin/bash

# a simple script to test hexray against all scenes. Assumed to be called from within the hexray main dir

for fn in data/*.hexray; do ./build/hexray $fn; done
