BEGIN {
    print "const char *PTT_PCF = "
    print "\"DEFAULT_OPTIONS\\n\\n\""
    print "\"LEVEL               THREAD\\n\""
    print "\"UNITS               NANOSEC\\n\""
    print "\"LOOK_BACK           100\\n\""
    print "\"SPEED               1\\n\""
    print "\"FLAG_ICONS          DISABLED\\n\""
    print "\"NUM_OF_STATE_COLORS 400\\n\""
    print "\"YMAX_SCALE          100\\n\\n\\n\""
    print "\"DEFAULT_SEMANTIC\\n\\n\""
    print "\"THREAD_FUNC          State As Is\\n\\n\\n\""
    print "\"EVENT_TYPE\\n\""
    print "\"0    70000001    Thread state\\n\""
    print "\"VALUES\\n\""
    print "\"0      Dead\\n\""
    print "\"1      Alive\\n\\n\\n\""
    print "\"EVENT_TYPE\\n\""
    print "\"4    70000002    Flushing trace buffer\\n\""
    print "\"VALUES\\n\""
    print "\"0      Finished\\n\\n\\n\""
    print "\"EVENT_TYPE\\n\""
    print "\"0    70000003    Pthread function\\n\""
    print "\"VALUES\\n\""
    print "\"0      User code\\n\""
}


/^\/\// { next }


{ printf "\"%d    %s\\n\"\n", NR, $2 }


END {
    print "\"\\n\\n\";"
}

