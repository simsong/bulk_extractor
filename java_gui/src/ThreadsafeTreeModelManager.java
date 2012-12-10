import javax.swing.SwingUtilities;
import javax.swing.tree.DefaultTreeModel;
import javax.swing.tree.MutableTreeNode;
import javax.swing.tree.TreeNode;
import java.lang.InterruptedException;
import java.lang.reflect.InvocationTargetException;


/**
 * The <code>ThreadsafeTreeModelManager</code> class provides a thread-safe wrapper
 * to DefaultTreeModel interfaces since these actions must be performed on the Swing thread.
 */
public class ThreadsafeTreeModelManager {
  public final DefaultTreeModel treeModel;

  // create the publicly accessible tree model
  public ThreadsafeTreeModelManager(TreeNode root) {
    treeModel = new DefaultTreeModel(root);
  }

  // thread-safe wrapper
  public void removeNodeFromParent(final MutableTreeNode node) {
    if (SwingUtilities.isEventDispatchThread()) {
      // in general usage, the thread is the event dispatch thread
      treeModel.removeNodeFromParent(node);
    } else {
      // schedule this immediately on the Swing dispatch thread
      try {
        SwingUtilities.invokeAndWait(new Runnable() {
          public void run() {
            treeModel.removeNodeFromParent(node);
          }
        });
      } catch (InterruptedException ie) {
        throw new RuntimeException("unexpected event");
      } catch (InvocationTargetException ite) {
        throw new RuntimeException("unexpected event");
      }
    }
  }

  // thread-safe wrapper
  public void insertNodeInto(final MutableTreeNode newChild, final MutableTreeNode parent, final int index) {
    if (SwingUtilities.isEventDispatchThread()) {
      // in general usage, the thread is the event dispatch thread
      treeModel.insertNodeInto(newChild, parent, index);
    } else {
      // schedule this immediately on the Swing dispatch thread
      try {
        SwingUtilities.invokeAndWait(new Runnable() {
          public void run() {
            treeModel.insertNodeInto(newChild, parent, index);
          }
        });
      } catch (InterruptedException ie) {
        throw new RuntimeException("unexpected event");
      } catch (InvocationTargetException ite) {
        throw new RuntimeException("unexpected event");
      }
    }
  }

  // thread-safe wrapper
  public void reload() {
    if (SwingUtilities.isEventDispatchThread()) {
      // in general usage, the thread is the event dispatch thread
      treeModel.reload();
    } else {
      // schedule this immediately on the Swing dispatch thread
      try {
        SwingUtilities.invokeAndWait(new Runnable() {
          public void run() {
            treeModel.reload();
          }
        });
      } catch (InterruptedException ie) {
        throw new RuntimeException("unexpected event");
      } catch (InvocationTargetException ite) {
        throw new RuntimeException("unexpected event");
      }
    }
  }
}

