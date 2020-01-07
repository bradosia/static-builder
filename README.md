# static-builder
Designed to build static libraries out of 3rd party projects without having to deal with their build system. Many build files included in projects are not able to be statically compiled as a library.

## Usage

1. Place static-builder.exe in the root folder you wish to make a static library of.
2. Run the program.
3. Deal with the errors by making tweaks to the source.

## configuration

static-builder.exe uses a file named static-builder-settings.json to specify user settings.

### Exclude a directory

```json
{
  "exclude" : {
    "/foo", "/bar"
  }
}
```
