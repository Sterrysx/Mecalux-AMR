#!/bin/bash
cd "/mnt/c/Users/APV/Documents/Abel/__Uni_Actual/0. PAE/Mecalux-AMR/backend"
exec stdbuf -oL -eL ./build/fleet_manager --cli --robots 1
