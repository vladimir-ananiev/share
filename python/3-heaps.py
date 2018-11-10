#!/usr/bin/python3

import random

x = random.randint(1,10)
y = random.randint(1,10)
z = random.randint(1,10)
xName = "Left"
yName = "Middle"
zName = "Right"

userStep = False

while True:
  print(xName,"=",x, yName,"=",y, zName,"=",z)
  print()
  if x==0 or y==0 or z==0:
    break
  userStep = not userStep
  if userStep:
    choice = int(input('Where from (1-Left, 2-Middle, 3-Right)? ').strip())
    count = int(input('How many? ').strip())
    if choice == 1:
      print("You take", count, "from", xName)
      x -= count
    elif choice == 2:
      print("You take", count, "from", yName)
      y -= count
    else:
      print("You take", count, "from", zName)
      z -= count
    if x<0 or y<0 or z<0:
      if x < 0:
        x += count
      elif y < 0:
        y += count
      else:
        z += count
      print("You take too many, please try again...")
      userStep = False
  else:
    if x == y:
      print("I take", z, "from", zName)
      z = 0
    elif y == z:
      print("I take", x, "from", xName)
      x = 0
    elif z == x:
      print("I take", y, "from", yName)
      y = 0
    else:
      if x>y and x>z:
        if y!=1 and z!=1:
          count = x-1
        elif y!=2 and z!=2:
          count = x-2
        else:
          count = 1
        print("I take", count, "from", xName)
        x -= count
      elif y>x and y>z:
        if x!=1 and z!=1:
          count = y-1
        elif x!=2 and z!=2:
          count = y-2
        else:
          count = 1
        print("I take", count, "from", yName)
        y -= count
      else:
        if x!=1 and y!=1:
          count = z-1
        elif x!=2 and y!=2:
          count = z-2
        else:
          count = 1
        print("I take", count, "from", zName)
        z -= count

if x == 0:
  x = y
  y = z
  xName = yName
  yName = zName
elif y == 0:
  y = z
  yName = zName

print("Continue with", xName, "and", yName, "...")

# Go on with 2 heaps

while True:
  print(xName,"=",x, yName,"=",y)
  print()
  if x==0 and y==0:
    break
  userStep = not userStep
  if userStep:
    choice = int(input('Where from (1-Left, 2-Right)? ').strip())
    count = int(input('How many? ').strip())
    if choice == 1:
      print("You take", count, "from", xName)
      x -= count
    else:
      print("You take", count, "from", yName)
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
      print("I take", x-y, "from", xName)
      x -= x-y
    elif y > x:
      print("I take", y-x, "from", yName)
      y -= y-x
    else:
      print("I take", 1, "from", xName)
      x -= 1

if userStep:
  print("You win, congratulations!!!")
else:
  print("I win, sorry:(")
