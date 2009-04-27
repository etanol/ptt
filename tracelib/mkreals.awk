BEGIN {
    print "#ifndef __ptt_realpthread"
    print "#define __ptt_realpthread"
    print ""
    print "#include <pthread.h>"
    print ""
}


/^\/\// { next }


{
    rtype = $1
    fname = $2
    argdecl = substr($0, index($0, $3))

    print "extern", rtype, "__real_" fname, argdecl ";"
}


END {
    print ""
    print "#endif /* __ptt_realpthread */"
}

