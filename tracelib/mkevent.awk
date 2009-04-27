BEGIN {
    print "EVENT_TYPE"
    print "0    70000003    Pthread function"
    print "VALUES"
    print "0      User code"
}


/^\/\// { next }


{ print NR, "    ", $2 }

