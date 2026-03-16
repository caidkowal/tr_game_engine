#include <check.h>
#include <stdlib.h>

//more suites
Suite *player_suite(void);
Suite *room_suite(void);
Suite *loader_suite(void);
Suite *engine_suite(void);

int main(void)
{
    Suite *suites[] = { 
        player_suite(),
        room_suite(),
        loader_suite(),
        engine_suite(),
        //more suites
        NULL
    };

    SRunner *runner = srunner_create(suites[0]);
    for (int i = 1; suites[i] != NULL; ++i) {
        srunner_add_suite(runner, suites[i]);
    }

    srunner_run_all(runner, CK_NORMAL);
    int failed = srunner_ntests_failed(runner);
    srunner_free(runner);

    return failed ? EXIT_FAILURE : EXIT_SUCCESS;
}
