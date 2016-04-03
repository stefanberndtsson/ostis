        DC.L $FA52235F          ; Cartridge header
        BKPT #0                 ; TEST_HOOK_INIT

        BKPT #6                 ; Setup Test 1
        MOVE.W D0,D1
        MOVE.W A0,D2
        BKPT #1                 ; Verify Test 1

        BKPT #6                 ; Setup Test 2
        MOVE.W D0,(A0)
        BKPT #1                 ; Verify Test 2

        BKPT #6                 ; Setup Test 3
        MOVE.W D0,(A0)+
        BKPT #1                 ; Verify Test 3

        BKPT #6                 ; Setup Test 4
        MOVE.W D0,-(A0)
        BKPT #1                 ; Verify Test 4

        BKPT #6                 ; Setup Test 5
        MOVE.W D0,4(A0)
        BKPT #1                 ; Verify Test 5

        BKPT #6                 ; Setup Test 6
        MOVE.W D0,4(A0,D1.W)
        BKPT #1                 ; Verify Test 6

        BKPT #6                 ; Setup Test 7
        MOVE.W D0,$4000.W
        BKPT #1                 ; Verify Test 7

        BKPT #6                 ; Setup Test 8
        MOVE.W D0,$14000.L
        BKPT #1                 ; Verify Test 8

        BKPT #7                 ; TEST_HOOK_EXIT

