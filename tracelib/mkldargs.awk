BEGIN {
    printf "ld_wrap := -Wl"
}


{
    printf ",--wrap,%s", $2
}


END {
    printf "\n"
}

