## Connection links

This file configures what a button state change (IN) will cause on a relay action (OUT).  

The first table you can ignore, it informs you (and the script) about which the valid values are in the 2nd table.



## Value Table

Please do edit this table, unless you're updating the firmware along with it!

| Commands       | States         | Actions        | Value          |
|----------------|----------------|----------------|----------------|
| C_NONE         | S_NONE         | A_NONE         | 0              |
| C_BUTTON       | S_PRESSED      | A_TOGGLE       | 1              |
| C_RELAY        | S_RELEASED     | A_ON           | 2              |
| C_TIMER        | S_SINGLE_CLICK | A_OFF          | 3              |
|                | S_DOUBLE_CLICK |                | 4              |
|                | S_TRIPLE_CLICK |                | 5              |
|                | S_LONG_PRESS   |                | 6              |
|                | S_ON           |                | 7              |
|                | S_OFF          |                | 8              |

## Data Table

Optionally you can add a `Comment` column to let you remember what it actually means.


| IN Commands | IN States      | IN Actions | IN Value | OUT Cmd    | OUT States     | OUT Actions | OUT Value |
|-------------|----------------|------------|----------|------------|----------------|-------------|-----------|
| C_BUTTON    | S_SINGLE_CLICK | A_NONE     | 31       | C_RELAY    | S_NONE         | A_TOGGLE    | 16        |
| C_BUTTON    | S_SINGLE_CLICK | A_NONE     | 30       | C_RELAY    | S_NONE         | A_TOGGLE    | 17        |
| C_BUTTON    | S_SINGLE_CLICK | A_NONE     | 29       | C_RELAY    | S_NONE         | A_TOGGLE    | 18        |
| C_BUTTON    | S_SINGLE_CLICK | A_NONE     | 28       | C_RELAY    | S_NONE         | A_TOGGLE    | 19        |
