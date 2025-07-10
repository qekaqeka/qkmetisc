local function bind_func(func, ...)
    local arg = {...}
    return function()
        func(unpack(arg))
    end
end

local function system(command)
    local res = vim.system(command, {text = true}):wait()

    print(res.stdout)

    if res.code ~= 0 then
        print(res.stderr)
        return false
    end
    return true
end

local function cmake_build_dir(root_path, build_dir_name, extra_options, extra_build_options)
    local build_path = root_path .. "/" .. build_dir_name
    if extra_options == nil then
        extra_options = {}
    end
    if extra_build_options == nil then
        extra_build_options = {}
    end

    if system{"cmake", "-S", root_path, "-B", build_path, unpack(extra_options)} then
        system{"cmake", "--build", build_path, unpack(extra_build_options)}
    end
end

local function build_one(file, outfile, options)
    if options == nil then
        options = {}
    end

    system{"cc", file, "-o", outfile, unpack(options)}
end

local build_type = {
    release = 1,
    debug = 0,
}

local function c_build(type)
    local workspace = vim.lsp.buf.list_workspace_folders()[1]

    if workspace ~= nil then
        vim.cmd "wa"
        if type == build_type.debug then
            cmake_build_dir(workspace, "build-debug", {"-DCMAKE_BUILD_TYPE=Debug"}, {})
        elseif type == build_type.release then
            cmake_build_dir(workspace, "build-release", {}, {})
        else
            error("unexpected build type")
        end
    else
        vim.cmd "w"
        local path = vim.fn.expand "%:p"
        local name = vim.fn.expand "%:r"
        if type == build_type.debug then
            build_one(path, name .. "-debug", {"-g"})
        elseif type == build_type.release then
            build_one(path, name)
        else
            error("unexpected build type")
        end
    end
end

local function clang_format()
    vim.cmd "w"
    system{"clang-format", "-i", "--style=file", vim.fn.expand "%:p"}
    vim.cmd "e"
end

local function c_filetype_callback()
    vim.keymap.set("n", "<C-k>", clang_format, {buffer = true})
    vim.keymap.set("n", "<C-b>", bind_func(c_build, build_type.debug), {buffer = true})
    vim.keymap.set("n", "<C-A-b>", bind_func(c_build, build_type.release), {buffer = true})
end

vim.api.nvim_create_autocmd({"FileType"},
    {
        pattern = {"c", "cpp"},
        callback = c_filetype_callback
    }
)

vim.api.nvim_create_user_command("Build", function(args)
        if args.nargs ~= 1 then
            error("Invalid args", 2)
        end

        if args.args == "debug" then
            c_build(build_type.debug)
        elseif args.args == "release" then
            c_build(build_type.release)
        else
            error("Invalid args", 2)
        end
    end,
    {desc = "Build project which is related to the current buffer"}
)

vim.api.nvim_create_user_command("CFormat", clang_format, {desc = "Format current buffer with clang format"})
