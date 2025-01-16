#!/bin/bash

find ./logs -type f -exec cat {} + | sort -n > logs/all