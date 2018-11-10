#!/usr/bin/python3

import random

x = random.randint(1,10)
y = random.randint(1,10)

userStep = False

while True:
  print("Left =",x, ", Right =", y)
  print()
  if x==0 and y==0:
    break
  userStep = not userStep
  if userStep:
    choice = int(input('Where from (1-Left, 2-Right)? ').strip())
    count = int(input('How many? ').strip())
    if choice == 1:
      print("You take", count, "from Left")
      x -= count
    else:
      print("You take", count, "from Right")
      y -= count
    if x<0 or y<0:
      if x < 0:
        x += count
      else:
        y += count
      print("You take too many, please try again...")
      userStep = False
  else:
    if x > y:
      print("I take", x-y, "from Left")
      x -= x-y
    elif y > x:
      print("I take", y-x, "from Right")
      y -= y-x
    else:
      print("I take", 1, "from Left")
      x -= 1

if userStep:
  print("You win, congratulations!!!")
else:
  print("I win, sorry:(")
