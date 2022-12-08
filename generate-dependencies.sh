#!/bin/sh

# $1 => destination file
# $2 => source_directory
# $3 => release_directory
# $4 => debug_directory
truncate --size=0 "${1}"
for source in ${2}/*.c; do
	dependencies=()
	delim="\""
	search=$(grep -o "#include \".*\"" "${source}")
	if [ "${search}" ]; then
		string="${search}${delim}"
		if [ "${string}" ]; then
			string=${string#*"${delim}"}
			while [[ "${string}" ]]; do
				test="${string%%"${delim}"*}"
				for name in "${dependencies[@]}"; do
					if [ "${name}" == "${test}" ]; then
						string=${string#*"${delim}"}
						string=${string#*"${delim}"}
						continue 2
					fi
				done
				dependencies+=( "${test}" )
				string=${string#*"${delim}"}
				string=${string#*"${delim}"}
			done
		fi
	fi

	checked=0
	while [ "${checked}" -lt "${#dependencies[@]}" ]; do
		search=$(grep -o "#include \".*\"" "${2}/${dependencies[checked]}")
		if [ "${search}" ]; then
			string="${search}${delim}"
			if [ "${string}" ]; then
				string=${string#*"${delim}"}
				while [[ "${string}" ]]; do
					test="${string%%"${delim}"*}"
					for name in "${dependencies[@]}"; do
						if [ "${name}" == "${test}" ]; then
							string=${string#*"${delim}"}
							string=${string#*"${delim}"}
							continue 2
						fi
					done
					dependencies+=( "${test}" )
					string=${string#*"${delim}"}
					string=${string#*"${delim}"}
				done
			fi
		fi
		((++checked))
	done

	file="${source}"
	file="${file##${2}/}"
	file="${file%%.c}"
	file="${file}.o"

	printf "${3}/${file}:" >> "${1}"
	for name in "${dependencies[@]}"; do
		printf " ${2}/${name}" >> "${1}"
	done
	printf "\n${4}/${file}:" >> "${1}"
	for name in "${dependencies[@]}"; do
		printf " ${2}/${name}" >> "${1}"
	done
	printf "\n" >> "${1}"
done
