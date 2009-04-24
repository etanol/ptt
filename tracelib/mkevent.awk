BEGIN {
    print "EVENT_TYPE"
    print "0    70000003    Pthread function"
    print "VALUES"
    print "0      User code"
}


{ print NR, "    ", $2 }

