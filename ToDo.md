## Reader Class
1. Wait for inotify events (like IN_MODIFY, IN_CREATE etc) to know if something happened with the file -- DONE
2. Then start reading until EAGAIN error code is returned by sys read() -- DONE
3. Keep a track of last_processed_offset, which should be updated by the number of bytes read. Also keep a track of an inotify
  event which tells you if the file has been deleted or moved since in linux(especially embedded devices) file rotations are done. -- DONE
  In such a case last_processed_offset should be set to zero(think more abt this). Inode can be obtained used fstat() after opening the file.
4. Put a small sleep in the reading loop to forecfully control the frequency of messages being sent to the Manager -- DONE

5. Add a utf-8 validation after reading data after an event trigger. Append an error event to the message queue so that frontend gets to know as well.
6. 


## Manager Class
1. Spawn threads for each file.
2. Create a thread safe queue shared by all instances of file reader.
3. Create another thread safe queue which pushes the message to the frontend
4. Manager class should process the messages from file reader and translate them into appropriate front end parallels.

___

## UI
In FTXUI:
1. Element - something that is created everytime during rendering and does not exist after a rendering loop.
2. Component - something that exists across rendering iterations. Use this for those things that require user interaction.

### Control Panel
####  Design
1. Option to add file(s) or a directory on startup.
  - Enable multiselect
  - Save button to confirm the selection
  - Use a separate ScreenInteractive Session for this
2. User should be able to add files while application is running.
3. Check Ftxui::Window for Control Panel's implementation. It gives nice options to show an item in focus and allows stacking multiple items. 

#### Implementation
1. Some kind of verification if the selected files are suitable for reading.
2. Better make add_file() a separate functionality.

### Viewport
#### Design
1. Root container - a vertical container to represent the entire visible area. 
  - This container has:
    - A header showing the Application name - Element
    - A close button - Component
      - Click sends command to FileManager to close all the threads and file_readers.
    - Wrapper containers each representing a row of horizontally stacked containers - Component
2. Wrapper Element - Element, hbox
  - It is just there to stack the File containers in a single row
  - Contains file containers
3. File container
  - A header showing the file name - Element
  - A close button - Component
    - Click sends command to File Manager to close thread and file_reader corresponding to this file using file_id.
  - A scroll bar - Component
    - No built in option, need a custom implementation.
  - Text area - Component (keeping it component instead of element because might want to add a feature on click of this text area.)

4. Grid organisation
  - Root container should be a vertical container
  - Wrapper container should be a horizontal container
  - Terminal can be resized during runtime so terminal size can only be known during runtime and hence the following logic to decide max rows and max cols should be executed during runtime too.
  - Keep adding file containers to a wrapper container uptil
    - The width of each file container is <= some constant  __C__ x terminal width. Use this to derive max cols.
    - When the above condition is false, create a new wrapper container and add new file containers to it.
  - Keep adding wrapper containers to root container uptil
    - The height of each wrapper container is <= some constant __C__ x terminal height. Use this to derive max rows.
    - When the above condition is false, notify user that no more files can be added now.
  - Hard limit on open panes. Derive this using max rows and cols.
  - The constant __C__ can be exposed as a user setting.

5. Pane Focus Management 
  
#### Events
1. Click on a pane should bring it into focus.
2. While in focus, 'q' should close that pane, otherwise nothing.
3. Shift + q just exit the application.

### Future features
1. Option to search files using regex and then multiselect.


## UI - Control Panel
1. Two major components: Searchable Menu and Selected Files Menu
2. Searchable Menu
  - Pressing spacebar in Searchable Menu should add it to the other Menu, but the file should still be visible in the Searchable Menu(less hassle and also useful if it is later removed from the other Menu).
  - Make sure the file is only added once (use a set).
  - If spacebar is pressed on a dir, notify the user that only a file can be added.
3. Selected Files Menu
  - Pressing spacebar in Selected Files Menu should remove the file.
4. Save/cancel buttons below these menus to start the application.

## ToDo 09/05/26
1. Check why sent data is not visible in the UI. -- DONE
2. Implement a separate logger for print statements so that UI can be shown independently. -- DONE

## ToDo 16/05/26
1. Topic name(s) based subscription.
2. File selector on app startup. Make it generic so that it can be used to add a file later on as well.
3. Split ControlPanel into model and view -- DONE
4. Split Viewport and UIManager into model and view as well.


## Nice features to have
1. Keyboard shortcut to move between searchable menu and Menu showing selected files.
2. Regex based directory scan so that user gets suggestions what directories are available.
