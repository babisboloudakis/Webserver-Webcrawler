#	 Charalambos Mpooloudakis
#    1115201500103
#    project3 - syspro

#1/bin/bash
echo "*Starting webcreator ..."

# Save command line arguments
root=$1
text=$2
w=$3
p=$4

# Validate command line arguments
integer='^[0-9]+$'
if ! [[ $w =~ $integer ]] ; then
    echo "error: w not integer"
    exit 1
fi
if ! [[ $p =~ $integer ]] ; then
    echo "error: p not integer"
    exit 2
fi
if ! [[ -f $text ]] ; then
    echo "error: text file doesn't exist"
    exit 3
fi
if ! [[ -d $root ]] ; then
    echo "error: root directory doesn't exist"
    exit 4
fi

# Make sure that the given file has
# more than 10000 lines of text
wc=($(wc $text))
lines=${wc[0]}
if [ $lines -lt 10000 ]; then
    echo "error: text file is too small"
    exit 5
fi

# Check if the directory has content.
# If so, empty it
if [ -n "$(ls -A ${root})" ] ; then
    echo "Warning: directory is full, purging ..."
    rm -r ${root}
    mkdir ${root}
fi

# List of all links to pages
# links are sorted by their website
# ( first are pages of website0, then website1 etc ...)
links=()

# Creation of all the websites and pages
# Also, filling of the links() array
# Note: pages created here are empty
root_i=0
while [ $root_i -lt $w ]
do
    # Creation of w websites
    path="${root}/site${root_i}"
    mkdir $path 
    text_i=0
    while [ $text_i -lt $p ]
    do  
        # Creation of p pages within the website
        fullpath="${path}/page${root_i}_${text_i}.html"
        touch $fullpath 
        links+=("../site${root_i}/page${root_i}_${text_i}")
        let "text_i++"
    done
    let "root_i++"
done

# Generate random page content
# Take every created page and fill it with
# text and links to other pages
# internal or external links randomly mixed
root_i=0
while [ $root_i -lt $w ]
do
    echo "* Creating website ${root_i} ..."    
    path="${root}/site${root_i}"
    text_i=0
    while [ $text_i -lt $p ]
    do
        fullpath="${path}/page${root_i}_${text_i}.html"

        # step1 - Pick a random starting line in the given file
        k=$((1+ $RANDOM % ( $lines - 2000 ) )) 

        # step2 - Pick a random number of lines
        m=$((1000 + $RANDOM % 1000 ))
        echo "*  Creating page ${fullpath} with $m lines starting at line $k ..."

        # step 3 - Create f in count, random internal links for pages within the same website
        f=$((p/2 +1)) 
        f_i=0
        intLinks=()
        while [ $f_i -lt $f ]
        do
            # Pick a random link that is within the same website
            randomIndex=$(($root_i * $p + $RANDOM % $p))
            element=${links[$randomIndex]}

            # If links to itself, find a new one
            if [[ "$element" == "../site${root_i}/page${root_i}_${text_i}" ]] ; then
                continue
            fi

            # If link already in page, find a new one
            found=0
            index=0
            while [ $index -lt ${#intLinks[@]} ]
            do
                if [[ "${intLinks[$index]}" == "$element" ]] ; then
                    found=1
                fi
                let "index++"
            done
            if [[ $found -eq 0 ]] ; then
                let "f_i++"
                intLinks+=($element)
            fi
        done
        
        # Step4 - Create q in count, random external links for pages from different websites
        tempLinks=()
        link_i=0

        # Find all external links from the links() array
        while [ $link_i -lt ${#links[@]} ]
        do
            if [ $link_i -ge $(($root_i * $p)) ] && [ $link_i -lt $(($root_i * $p + $p)) ] ; then
                let "link_i++"
                continue
            fi
            tempLinks+=(${links[$link_i]})
            let "link_i++"
        done

        q=$((w/2 +1))
        q_i=0
        extLinks=()
        # Pick random links from the tempLinks() array
        # links in tempLinks() array can't show to the page itself
        # as they are all external
        while [ $q_i -lt $q ]
        do
            randomIndex=$(($RANDOM % ${#tempLinks[@]}))
            element=${tempLinks[$randomIndex]}

            # If link already in page, find a new one
            found=0
            index=0
            while [ $index -lt ${#extLinks[@]} ]
            do
                if [[ "${extLinks[$index]}" == "$element" ]] ; then
                    found=1
                fi
                let "index++"
            done
            if [[ $found -eq 0 ]] ; then
                let "q_i++"
                extLinks+=($element)
            fi
        done
        
        # Step 5 - Generate HTML headers
        echo "<!DOCTYPE html>" >> $fullpath
        echo "<html>" >> $fullpath
        echo "  <body>" >> $fullpath
        
        # Step 6 and 7 - Mix external with internal links
        # then, insert them into the page along with random
        # text from the given file
        mixedLinks=("${intLinks[@]}" "${extLinks[@]}")
        mixedLinks=($(shuf -e "${mixedLinks[@]}"))
        content_i=0
        len=$(($m/($f+$q)))
        while [ $content_i -lt ${#mixedLinks[@]} ]
        do
            head -$((k + $len * $content_i)) ${text} | tail -${len} >> $fullpath
            mixedLinks[content_i]="${mixedLinks[$content_i]}.html"
            echo "*   Adding link to ${mixedLinks[$content_i]}"
            echo "      <a href=\"${mixedLinks[${content_i}]}\">${mixedLinks[${content_i}]}</a> " >> $fullpath
            let "content_i++"
        done

        # Step 8 - Generate HTML closing tags
        echo "  </body>" >> $fullpath
        echo "</html>" >> $fullpath

        let "text_i++"
    done
    let "root_i++"
done

echo "*All pages have at least one incoming link ..."
echo "*Webcreator done ..."