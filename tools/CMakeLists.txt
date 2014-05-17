add_executable(bin2c bin2c.c)

function(bin2c_target tag input output)
    add_custom_command(
        OUTPUT ${output}
        COMMAND $<TARGET_FILE:bin2c> ${tag} < ${input} > ${output}
        DEPENDS ${input} bin2c
    )
endfunction(bin2c_target)
