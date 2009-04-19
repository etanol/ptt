BEGIN {
    printf "LD_WRAP := -Wl"
}


{
    printf ",--wrap,%s", $2
}


END {
    printf "\n"
}

