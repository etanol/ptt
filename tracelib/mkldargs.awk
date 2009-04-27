BEGIN {
    printf "ld_wrap := -Wl"
}


/^\/\// { next }


{
    printf ",--wrap,%s", $2
}


END {
    printf "\n"
}

