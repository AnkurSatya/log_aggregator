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
