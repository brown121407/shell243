/* Shell
 *
 * Copyright 2021 © Alexandru-Sergiu Marton
 * Copyright 2021 © Daria Mihaela Broscoțeanu
 * Copyright 2021 © Gherghescu Andreea Diana
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

int main() {
    while (1) {
        char *line = readline("$ ");
        if (!line) {
            break;
        }
        if (*line) {
            add_history(line);
            split_arguments_by_space(line);
            if (pipes_cnt == 1)
                shell_run();
            else
                shell_run_pipes();
        }

        free(line);
    }

    return 0;
}
