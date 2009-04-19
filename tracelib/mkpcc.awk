BEGIN {
    wrapargs = ""
    wrapbuild = 1
}


/^::@@::@@::$/ {
    wrapbuild = 0
    wrapargs = substr(wrapargs, 2)
    next
}


/^wrap_args=@@@$/ {
    if (!wrapbuild) {
        print "wrap_args='" wrapargs "'"
        next
    }
}


{
    if (wrapbuild)
        wrapargs = wrapargs ",--wrap," $2
    else
        print
}

