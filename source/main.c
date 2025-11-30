/**
 * @file main.cpp
 * @author khalilhenoud@gmail.com
 * @brief
 * @version 0.1
 * @date 2023-01-20
 *
 * @copyright Copyright (c) 2023
 *
 */
#include <assert.h>
#include <stdio.h>
#include <game/game.h>
#include <library/os/os.h>

#define PERCENTAGE_WIDTH      0.75f
#define ASPECT_RATIO          (9.f/16.f)


// TODO: add json config file to control all these specs, window width, height
// style, etc...
int
main(int argc, char *argv[])
{
  assert(argc >= 2);

  {
    uint64_t result = 0;
    int32_t client_width = (int32_t)(get_screen_width() * PERCENTAGE_WIDTH);
    int32_t client_height = (int32_t)(client_width * ASPECT_RATIO);

    // NOTE: this is not very secure.
    char *cmd_params = argv[1];
    char data_dir[128];
    sscanf(cmd_params, " %s ", data_dir);

    game_init(client_width, client_height, data_dir);
    result = game_update();
    game_cleanup();
    return result;
  }
}
