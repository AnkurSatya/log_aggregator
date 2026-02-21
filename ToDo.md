## Reader Class
1. Wait for inotify events (like IN_MODIFY, IN_CREATE etc) to know if something happened with the file.
2. Then start reading (async) under EAGAIN error code is returned by sys read().
3. Keep a track of last_processed_offset, which should be updated by the number of bytes read. Also keep a track of an inotify
  event which tells you if the file has been deleted or moved since in linux(especially embedded devices) file rotations are done. 
  In such a case last_processed_offset should be set to zero(think more abt this). Inode can be obtained used fstat() after opening the file.
3. Put a small sleep in the reading loop to forecfully control the frequency of messages being sent to the Manager.


## Manager Class
1. Spawn threads for each file but only when IN_CREATE for a file has been fired.
2.
