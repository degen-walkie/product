import Link from 'next/link';
import { PageHero } from '@/components/ui';

export default function NotFound() {
  return (
    <>
      <PageHero eyebrow="404" title={<>No such<br /><span className="neon">room.</span></>} lead="Roll back to the list." ctas={<Link className="btn btn-neon" href="/">Home</Link>} />
    </>
  );
}
