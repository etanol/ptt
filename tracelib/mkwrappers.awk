BEGIN {
    autoname[1] = "a_"
    autoname[2] = "b_"
    autoname[3] = "c_"
    autoname[4] = "d_"
    autoname[5] = "e_"
    autoname[6] = "f_"
    autoname[7] = "g_"
    autoname[8] = "h_"

    print "#include \"reals.h\""
    print "#include \"core.h\""
    print "#include \"ptt.h\""
    print ""
    print ""
}


{
    rtype = $1
    fname = $2
    fid = NR
    argdecl = substr($0, index($0, $3))

    # Split arguments, give them names and join them for prototype and call
    argcall = ""
    argproto = ""
    if (argdecl == "(void)")
        argproto = "void"
    else {
        argcount = split(substr(argdecl, 2, length(argdecl) - 2), arg, /, */)
        for (i = 1;  i <= argcount;  i++) {
            argcall = argcall ", " autoname[i]
            if (match(arg[i], /\(\*\)/)) {
                sub(/\(\*\)/, "(*" autoname[i] ")", arg[i])
                argproto = argproto ", " arg[i]
            } else
                argproto = argproto ", " arg[i] " " autoname[i]
        }
        argcall = substr(argcall, 3)
        argproto = substr(argproto, 3)
    }

    print rtype, "__wrap_" fname, "(" argproto ")"
    print "{"
    if (rtype != "void") {
        print "       ", rtype, "r_;"
        print ""
    }
    print "        ptt_event(PTT_EVENT_PTHREAD_FUNC,", fid ");"
    if (rtype != "void")
        print "        r_ = __real_" fname "(" argcall ");"
    else
        print "        __real_" fname "(" argcall ");"
    print "        ptt_event(PTT_EVENT_PTHREAD_FUNC, 0);"
    if (rtype != "void")
        print "        return r_;"
    print "}"
    print ""
    print ""
}

