/*
 * Copyright (C) 2007, 2009 Apple Inc. All rights reserved.
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include "sky/engine/core/editing/DOMSelection.h"

#include "sky/engine/bindings/exception_messages.h"
#include "sky/engine/bindings/exception_state.h"
#include "sky/engine/bindings/exception_state_placeholder.h"
#include "sky/engine/core/dom/Document.h"
#include "sky/engine/core/dom/ExceptionCode.h"
#include "sky/engine/core/dom/Node.h"
#include "sky/engine/core/dom/Range.h"
#include "sky/engine/core/dom/TreeScope.h"
#include "sky/engine/core/editing/FrameSelection.h"
#include "sky/engine/core/editing/TextIterator.h"
#include "sky/engine/core/editing/htmlediting.h"
#include "sky/engine/core/frame/LocalFrame.h"
#include "sky/engine/core/inspector/ConsoleMessage.h"
#include "sky/engine/wtf/text/WTFString.h"

namespace blink {

static Node* selectionShadowAncestor(LocalFrame* frame)
{
    Node* node = frame->selection().selection().base().anchorNode();
    if (!node)
        return 0;

    if (!node->isInShadowTree())
        return 0;

    return frame->document()->ancestorInThisScope(node);
}

DOMSelection::DOMSelection(const TreeScope* treeScope)
    : DOMWindowProperty(treeScope->rootNode().document().frame())
    , m_treeScope(treeScope)
{
}

void DOMSelection::clearTreeScope()
{
    m_treeScope = nullptr;
}

const VisibleSelection& DOMSelection::visibleSelection() const
{
    ASSERT(m_frame);
    return m_frame->selection().selection();
}

static Position anchorPosition(const VisibleSelection& selection)
{
    Position anchor = selection.isBaseFirst() ? selection.start() : selection.end();
    return anchor.parentAnchoredEquivalent();
}

static Position focusPosition(const VisibleSelection& selection)
{
    Position focus = selection.isBaseFirst() ? selection.end() : selection.start();
    return focus.parentAnchoredEquivalent();
}

static Position basePosition(const VisibleSelection& selection)
{
    return selection.base().parentAnchoredEquivalent();
}

static Position extentPosition(const VisibleSelection& selection)
{
    return selection.extent().parentAnchoredEquivalent();
}

Node* DOMSelection::anchorNode() const
{
    if (!m_frame)
        return 0;

    return shadowAdjustedNode(anchorPosition(visibleSelection()));
}

int DOMSelection::anchorOffset() const
{
    if (!m_frame)
        return 0;

    return shadowAdjustedOffset(anchorPosition(visibleSelection()));
}

Node* DOMSelection::focusNode() const
{
    if (!m_frame)
        return 0;

    return shadowAdjustedNode(focusPosition(visibleSelection()));
}

int DOMSelection::focusOffset() const
{
    if (!m_frame)
        return 0;

    return shadowAdjustedOffset(focusPosition(visibleSelection()));
}

Node* DOMSelection::baseNode() const
{
    if (!m_frame)
        return 0;

    return shadowAdjustedNode(basePosition(visibleSelection()));
}

int DOMSelection::baseOffset() const
{
    if (!m_frame)
        return 0;

    return shadowAdjustedOffset(basePosition(visibleSelection()));
}

Node* DOMSelection::extentNode() const
{
    if (!m_frame)
        return 0;

    return shadowAdjustedNode(extentPosition(visibleSelection()));
}

int DOMSelection::extentOffset() const
{
    if (!m_frame)
        return 0;

    return shadowAdjustedOffset(extentPosition(visibleSelection()));
}

bool DOMSelection::isCollapsed() const
{
    if (!m_frame || selectionShadowAncestor(m_frame))
        return true;
    return !m_frame->selection().isRange();
}

String DOMSelection::type() const
{
    if (!m_frame)
        return String();

    FrameSelection& selection = m_frame->selection();

    // This is a WebKit DOM extension, incompatible with an IE extension
    // IE has this same attribute, but returns "none", "text" and "control"
    // http://msdn.microsoft.com/en-us/library/ms534692(VS.85).aspx
    if (selection.isNone())
        return "None";
    if (selection.isCaret())
        return "Caret";
    return "Range";
}

int DOMSelection::rangeCount() const
{
    if (!m_frame)
        return 0;
    return m_frame->selection().isNone() ? 0 : 1;
}

void DOMSelection::collapse(Node* node, int offset, ExceptionState& exceptionState)
{
    ASSERT(node);
    if (!m_frame)
        return;

    if (offset < 0) {
        exceptionState.ThrowDOMException(IndexSizeError, String::number(offset) + " is not a valid offset.");
        return;
    }

    if (!isValidForPosition(node))
        return;
    RefPtr<Range> range = Range::create(node->document());
    range->setStart(node, offset, exceptionState);
    if (exceptionState.had_exception())
        return;
    range->setEnd(node, offset, exceptionState);
    if (exceptionState.had_exception())
        return;
    m_frame->selection().setSelectedRange(range.get(), DOWNSTREAM, m_frame->selection().isDirectional() ? FrameSelection::Directional : FrameSelection::NonDirectional);
}

void DOMSelection::collapseToEnd(ExceptionState& exceptionState)
{
    if (!m_frame)
        return;

    const VisibleSelection& selection = m_frame->selection().selection();

    if (selection.isNone()) {
        exceptionState.ThrowDOMException(InvalidStateError, "there is no selection.");
        return;
    }

    m_frame->selection().moveTo(VisiblePosition(selection.end(), DOWNSTREAM));
}

void DOMSelection::collapseToStart(ExceptionState& exceptionState)
{
    if (!m_frame)
        return;

    const VisibleSelection& selection = m_frame->selection().selection();

    if (selection.isNone()) {
        exceptionState.ThrowDOMException(InvalidStateError, "there is no selection.");
        return;
    }

    m_frame->selection().moveTo(VisiblePosition(selection.start(), DOWNSTREAM));
}

void DOMSelection::empty()
{
    if (!m_frame)
        return;
    m_frame->selection().clear();
}

void DOMSelection::setBaseAndExtent(Node* baseNode, int baseOffset, Node* extentNode, int extentOffset, ExceptionState& exceptionState)
{
    if (!m_frame)
        return;

    if (baseOffset < 0) {
        exceptionState.ThrowDOMException(IndexSizeError, String::number(baseOffset) + " is not a valid base offset.");
        return;
    }

    if (extentOffset < 0) {
        exceptionState.ThrowDOMException(IndexSizeError, String::number(extentOffset) + " is not a valid extent offset.");
        return;
    }

    if (!isValidForPosition(baseNode) || !isValidForPosition(extentNode))
        return;

    // FIXME: Eliminate legacy editing positions
    VisiblePosition visibleBase = VisiblePosition(createLegacyEditingPosition(baseNode, baseOffset), DOWNSTREAM);
    VisiblePosition visibleExtent = VisiblePosition(createLegacyEditingPosition(extentNode, extentOffset), DOWNSTREAM);

    m_frame->selection().moveTo(visibleBase, visibleExtent);
}

void DOMSelection::modify(const String& alterString, const String& directionString, const String& granularityString)
{
    if (!m_frame)
        return;

    FrameSelection::EAlteration alter;
    if (equalIgnoringCase(alterString, "extend"))
        alter = FrameSelection::AlterationExtend;
    else if (equalIgnoringCase(alterString, "move"))
        alter = FrameSelection::AlterationMove;
    else
        return;

    SelectionDirection direction;
    if (equalIgnoringCase(directionString, "forward"))
        direction = DirectionForward;
    else if (equalIgnoringCase(directionString, "backward"))
        direction = DirectionBackward;
    else if (equalIgnoringCase(directionString, "left"))
        direction = DirectionLeft;
    else if (equalIgnoringCase(directionString, "right"))
        direction = DirectionRight;
    else
        return;

    TextGranularity granularity;
    if (equalIgnoringCase(granularityString, "character"))
        granularity = CharacterGranularity;
    else if (equalIgnoringCase(granularityString, "word"))
        granularity = WordGranularity;
    else if (equalIgnoringCase(granularityString, "sentence"))
        granularity = SentenceGranularity;
    else if (equalIgnoringCase(granularityString, "line"))
        granularity = LineGranularity;
    else if (equalIgnoringCase(granularityString, "paragraph"))
        granularity = ParagraphGranularity;
    else if (equalIgnoringCase(granularityString, "lineboundary"))
        granularity = LineBoundary;
    else if (equalIgnoringCase(granularityString, "sentenceboundary"))
        granularity = SentenceBoundary;
    else if (equalIgnoringCase(granularityString, "paragraphboundary"))
        granularity = ParagraphBoundary;
    else if (equalIgnoringCase(granularityString, "documentboundary"))
        granularity = DocumentBoundary;
    else
        return;

    m_frame->selection().modify(alter, direction, granularity);
}

void DOMSelection::extend(Node* node, int offset, ExceptionState& exceptionState)
{
    ASSERT(node);

    if (!m_frame)
        return;

    if (offset < 0) {
        exceptionState.ThrowDOMException(IndexSizeError, String::number(offset) + " is not a valid offset.");
        return;
    }
    if (offset > (node->offsetInCharacters() ? caretMaxOffset(node) : (int)node->countChildren())) {
        exceptionState.ThrowDOMException(IndexSizeError, String::number(offset) + " is larger than the given node's length.");
        return;
    }

    if (!isValidForPosition(node))
        return;

    // FIXME: Eliminate legacy editing positions
    m_frame->selection().setExtent(VisiblePosition(createLegacyEditingPosition(node, offset), DOWNSTREAM));
}

PassRefPtr<Range> DOMSelection::getRangeAt(int index, ExceptionState& exceptionState)
{
    if (!m_frame)
        return nullptr;

    if (index < 0 || index >= rangeCount()) {
        exceptionState.ThrowDOMException(IndexSizeError, String::number(index) + " is not a valid index.");
        return nullptr;
    }

    // If you're hitting this, you've added broken multi-range selection support
    ASSERT(rangeCount() == 1);

    if (Node* shadowAncestor = selectionShadowAncestor(m_frame)) {
        ASSERT(!shadowAncestor->isShadowRoot());
        ContainerNode* container = shadowAncestor->parentOrShadowHostNode();
        int offset = shadowAncestor->nodeIndex();
        return Range::create(shadowAncestor->document(), container, offset, container, offset);
    }

    return m_frame->selection().firstRange();
}

void DOMSelection::removeAllRanges()
{
    if (!m_frame)
        return;
    m_frame->selection().clear();
}

void DOMSelection::addRange(Range* newRange)
{
    if (!m_frame)
        return;

    FrameSelection& selection = m_frame->selection();

    if (selection.isNone()) {
        selection.setSelectedRange(newRange, VP_DEFAULT_AFFINITY);
        return;
    }

    RefPtr<Range> originalRange = selection.firstRange();

    // FIXME: "Merge the ranges if they intersect" is Blink-specific behavior; other browsers supporting discontiguous
    // selection (obviously) keep each Range added and return it in getRangeAt(). But it's unclear if we can really
    // do the same, since we don't support discontiguous selection. Further discussions at
    // <https://code.google.com/p/chromium/issues/detail?id=353069>.

    Range* start = originalRange->compareBoundaryPoints(Range::START_TO_START, newRange, ASSERT_NO_EXCEPTION) < 0 ? originalRange.get() : newRange;
    Range* end = originalRange->compareBoundaryPoints(Range::END_TO_END, newRange, ASSERT_NO_EXCEPTION) < 0 ? newRange : originalRange.get();
    RefPtr<Range> merged = Range::create(originalRange->startContainer()->document(), start->startContainer(), start->startOffset(), end->endContainer(), end->endOffset());
    EAffinity affinity = selection.selection().affinity();
    selection.setSelectedRange(merged.get(), affinity);
}

void DOMSelection::deleteFromDocument()
{
    if (!m_frame)
        return;

    FrameSelection& selection = m_frame->selection();

    if (selection.isNone())
        return;

    RefPtr<Range> selectedRange = selection.selection().toNormalizedRange();
    if (!selectedRange)
        return;

    selectedRange->deleteContents(ASSERT_NO_EXCEPTION);

    setBaseAndExtent(selectedRange->startContainer(), selectedRange->startOffset(), selectedRange->startContainer(), selectedRange->startOffset(), ASSERT_NO_EXCEPTION);
}

bool DOMSelection::containsNode(const Node* n, bool allowPartial) const
{
    if (!m_frame)
        return false;

    FrameSelection& selection = m_frame->selection();

    if (!n || m_frame->document() != n->document() || selection.isNone())
        return false;

    unsigned nodeIndex = n->nodeIndex();
    RefPtr<Range> selectedRange = selection.selection().toNormalizedRange();

    ContainerNode* parentNode = n->parentNode();
    if (!parentNode)
        return false;

    TrackExceptionState exceptionState;
    bool nodeFullySelected = Range::compareBoundaryPoints(parentNode, nodeIndex, selectedRange->startContainer(), selectedRange->startOffset(), exceptionState) >= 0 && !exceptionState.had_exception()
        && Range::compareBoundaryPoints(parentNode, nodeIndex + 1, selectedRange->endContainer(), selectedRange->endOffset(), exceptionState) <= 0 && !exceptionState.had_exception();
    if (exceptionState.had_exception())
        return false;
    if (nodeFullySelected)
        return true;

    bool nodeFullyUnselected = (Range::compareBoundaryPoints(parentNode, nodeIndex, selectedRange->endContainer(), selectedRange->endOffset(), exceptionState) > 0 && !exceptionState.had_exception())
        || (Range::compareBoundaryPoints(parentNode, nodeIndex + 1, selectedRange->startContainer(), selectedRange->startOffset(), exceptionState) < 0 && !exceptionState.had_exception());
    ASSERT(!exceptionState.had_exception());
    if (nodeFullyUnselected)
        return false;

    return allowPartial || n->isTextNode();
}

void DOMSelection::selectAllChildren(Node* n, ExceptionState& exceptionState)
{
    if (!n)
        return;

    // This doesn't (and shouldn't) select text node characters.
    setBaseAndExtent(n, 0, n, n->countChildren(), exceptionState);
}

String DOMSelection::toString()
{
    if (!m_frame)
        return String();

    return plainText(m_frame->selection().selection().toNormalizedRange().get());
}

Node* DOMSelection::shadowAdjustedNode(const Position& position) const
{
    if (position.isNull())
        return 0;

    Node* containerNode = position.containerNode();
    Node* adjustedNode = m_treeScope->ancestorInThisScope(containerNode);

    if (!adjustedNode)
        return 0;

    if (containerNode == adjustedNode)
        return containerNode;

    ASSERT(!adjustedNode->isShadowRoot());
    return adjustedNode->parentOrShadowHostNode();
}

int DOMSelection::shadowAdjustedOffset(const Position& position) const
{
    if (position.isNull())
        return 0;

    Node* containerNode = position.containerNode();
    Node* adjustedNode = m_treeScope->ancestorInThisScope(containerNode);

    if (!adjustedNode)
        return 0;

    if (containerNode == adjustedNode)
        return position.computeOffsetInContainerNode();

    return adjustedNode->nodeIndex();
}

bool DOMSelection::isValidForPosition(Node* node) const
{
    ASSERT(m_frame);
    if (!node)
        return true;
    return node->document() == m_frame->document();
}

} // namespace blink