BEGIN {
    parsing = 0
    print "/* Automatically generated.  Do not edit. */"
}


/^\s*$/ {
    parsing = 0
    next
}


/^EVENT_TYPE\s*$/ {
    parsing = 1
    next
}


/^VALUES\s*$/ {
    parsing = 2
    next
}


{
    if (parsing == 1) {
        value = $2
        caption = substr($0, index($0, $3))
    } else if (parsing == 2) {
        value = $1
        caption = substr($0, index($0, $2))
    } else if (parsing == 0) {
        next
    }
    macro = toupper(caption)
    gsub(/ +/, "_", macro)
    printf "#define %s  %d\n", macro, value
}

