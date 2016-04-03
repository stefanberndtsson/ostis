        DC.L $FA52235F          ; Cartridge header
        BKPT #0                 ; TEST_HOOK_INIT

        BKPT #6                 ; Setup Test 1
        MOVE.W (A0),D0
        BKPT #1                 ; Verify Test 1

        BKPT #6                 ; Setup Test 2
        MOVE.W (A0)+,D0
        BKPT #1                 ; Verify Test 2

        BKPT #6                 ; Setup Test 3
        MOVE.W -(A0),D0
        BKPT #1                 ; Verify Test 3

        BKPT #6                 ; Setup Test 4
        MOVE.W 4(A0),D0
        BKPT #1                 ; Verify Test 4

        BKPT #6                 ; Setup Test 5
        MOVE.W 4(A0,D1.W),D0
        BKPT #1                 ; Verify Test 5

        BKPT #6                 ; Setup Test 6
        MOVE.W $4000.W,D0
        BKPT #1                 ; Verify Test 6

        BKPT #6                 ; Setup Test 7
        MOVE.W $14000.L,D0
        BKPT #1                 ; Verify Test 7

        BKPT #6                 ; Setup Test 8
        MOVE.W 4(PC),D0
        BKPT #1                 ; Verify Test 8

        BKPT #6                 ; Setup Test 9
        MOVE.W 4(PC,D1.W),D0
        BKPT #1                 ; Verify Test 9

        BKPT #6                 ; Setup Test 10
        MOVE.W #$11112345,D0
        BKPT #1                 ; Verify Test 10

        BKPT #7                 ; TEST_HOOK_EXIT

