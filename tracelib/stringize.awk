BEGIN {
    print "/* Automatically generated.  Do not edit. */"
    print "const char *PttPCF ="
}


{
    print "\"" $0 "\\n\""
}


END {
    print ";"
}

